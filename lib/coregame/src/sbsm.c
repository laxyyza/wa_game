#include "sbsm.h"
#include "coregame.h"
#include <string.h>

cg_sbsm_t* 
sbsm_create(u32 size, f64 interval_ms)
{
	u32 sbsm_size = sizeof(cg_sbsm_t) + (sizeof(cg_game_snapshot_t) * size);
	cg_sbsm_t* sbsm = calloc(1, sbsm_size);
	sbsm->size = size;
	sbsm->interval_ms = interval_ms;
	
	sbsm->base = sbsm->present = sbsm->snapshots;

	for (u32 i = 0; i < size; i++)
	{
		cg_game_snapshot_t* ss = sbsm->snapshots + i;
		ght_init(&ss->player_states, 10, free);
		ght_init(&ss->bullet_states, 100, free);
	}

	sbsm_rotate(NULL, sbsm);

	return sbsm;
}

static inline u32
sbsm_index(const cg_sbsm_t* sbsm, f64 timestamp_ms)
{
	return (u32)ceil(timestamp_ms / sbsm->interval_ms) % sbsm->size;
}

cg_game_snapshot_t* 
sbsm_lookup(cg_sbsm_t* sbsm, f64 timestamp_ms)
{
	cg_game_snapshot_t* ret = NULL;

	timestamp_ms = clampf64(timestamp_ms, sbsm->base->timestamp, sbsm->present->timestamp);
	u32 index = sbsm_index(sbsm, timestamp_ms);

	ret = sbsm->snapshots + index;

	return ret;
}

void
sbsm_commit_player(cg_game_snapshot_t* ss, cg_player_t* player)
{
	cg_player_snapshot_t* pss;

	if (player->dirty == false)
	{
		ght_del(&ss->player_states, player->id);
		return;
	}

	pss = ght_get(&ss->player_states, player->id);

	if (pss == NULL)
	{
		pss = calloc(1, sizeof(cg_player_snapshot_t));
		pss->player_id = player->id;
		ght_insert(&ss->player_states, pss->player_id, pss);
	}
	
	sbsm_player_to_snapshot(pss, player);
	pss->dirty = false;
	player->dirty = false;
}

void
sbsm_commit_bullet(cg_game_snapshot_t* ss, cg_bullet_t* bullet)
{
	cg_bullet_snapshot_t* bss = ght_get(&ss->bullet_states, bullet->id);
	if (bss == NULL)
	{
		bss = malloc(sizeof(cg_player_snapshot_t));
		bss->bullet_id = bullet->id;
		ght_insert(&ss->bullet_states, bss->bullet_id, bss);
	}
	bss->pos = bullet->r.pos;
	bss->collided = bullet->collided;
}

void 
sbsm_player_to_snapshot(cg_player_snapshot_t* pss, const cg_player_t* player)
{
	pss->pos = player->pos;
	pss->velocity = player->velocity;
	pss->input = player->input;
	pss->shooting = player->shoot;

	if (player->gun == NULL)
		return;

	pss->gun_id = player->gun->spec->id;
	pss->ammo = player->gun->ammo;
	pss->bullet_timer = player->gun->bullet_timer;
	pss->charge_timer = player->gun->charge_time;
	pss->reload_timer = player->gun->reload_time;
}

void
sbsm_snapshot_to_player(coregame_t* cg, cg_player_t* player, const cg_player_snapshot_t* pss)
{
	player->pos = pss->pos;
	player->pos.x -= pss->velocity.x;
	player->pos.y -= pss->velocity.y;
	player->input = pss->input;
	player->shoot = pss->shooting;

	if (pss->gun_id != player->gun->spec->id)
		coregame_player_change_gun_force(cg, player, player->gun->spec->id);

	if (player->gun->ammo != pss->ammo)
		player->gun_dirty = true;
	player->gun->ammo = pss->ammo;
	player->gun->bullet_timer = pss->bullet_timer;
	player->gun->charge_time = pss->charge_timer;
	player->gun->reload_time = pss->reload_timer;
}

void 
sbsm_snapshot_to_bullet(cg_bullet_t* bullet, const cg_bullet_snapshot_t* bss)
{
	bullet->r.pos = bss->pos;
	bullet->collided = bss->collided;
}

static inline void
sbsm_rewind_player(coregame_t* cg, cg_game_snapshot_t* gss, cg_player_t* player)
{
	cg_player_snapshot_t* pss = ght_get(&gss->player_states, player->id);

	if (pss && pss->dirty)
	{
		coregame_set_player_input(player, pss->input);
		pss->dirty = false;
	}

	if (player->gun)
		coregame_gun_update(cg, player->gun);

	player->velocity.x = player->dir.x * PLAYER_SPEED;
	player->velocity.y = player->dir.y * PLAYER_SPEED;

	coregame_update_player(cg, player);

	sbsm_commit_player(gss, player);
}

static inline void
sbsm_rewind_players(coregame_t* cg, cg_game_snapshot_t* gss)
{
	ght_t* players = &cg->players;

	GHT_FOREACH(cg_player_t* player, players, 
	{
		sbsm_rewind_player(cg, gss, player);
	});
}

static inline void
sbsm_rewind_bullets(coregame_t* cg, cg_game_snapshot_t* gss)
{
	ght_t* bullets = &cg->bullets;

	GHT_FOREACH(cg_bullet_t* bullet, bullets, 
	{
		coregame_update_bullet(cg, bullet);

		sbsm_commit_bullet(gss, bullet);
	});
}

static void 
sbsm_rewind(coregame_t* cg, cg_game_snapshot_t* gss)
{
	cg_sbsm_t* sbsm = cg->sbsm;
	u32 index = sbsm_index(sbsm, gss->timestamp);

	cg->rewinding = true;

	while (gss->timestamp <= sbsm->present->timestamp)
	{
		sbsm_rewind_players(cg, gss);
		sbsm_rewind_bullets(cg, gss);

		if (gss == sbsm->present)
			break;

		index++;
		if (index >= sbsm->size)
			index = 0;
		gss = sbsm->snapshots + index;
	}

	cg->rewinding = false;
}

static inline void
sbsm_rollback_player(coregame_t* cg, cg_player_snapshot_t* pss)
{
	cg_player_t* player = ght_get(&cg->players, pss->player_id);
	if (player)
	{
		sbsm_snapshot_to_player(cg, player, pss);
		coregame_set_player_input(player, pss->input);
		pss->dirty = false;
	}
}

static inline void
sbsm_rollback_players(coregame_t* cg, cg_game_snapshot_t* gss)
{
	ght_t* player_states = &gss->player_states;
	GHT_FOREACH(cg_player_snapshot_t* pss, player_states, 
	{
		sbsm_rollback_player(cg, pss);
	});
}

static inline void
sbsm_rollback_bullet(coregame_t* cg, cg_bullet_snapshot_t* bss)
{
	cg_bullet_t* bullet = ght_get(&cg->bullets, bss->bullet_id);
	if (bullet)
	{
		sbsm_snapshot_to_bullet(bullet, bss);
	}
}

static inline void
sbsm_rollback_bullets(coregame_t* cg, cg_game_snapshot_t* gss)
{
	ght_t* bullet_states = &gss->bullet_states;
	GHT_FOREACH(cg_bullet_snapshot_t* bss, bullet_states, 
	{
		sbsm_rollback_bullet(cg, bss);
	});
}

void 
sbsm_rollback(coregame_t* cg)
{
	cg_sbsm_t* sbsm = cg->sbsm;
	cg_game_snapshot_t* gss = sbsm->oldest_change;

	sbsm->oldest_change = NULL;

	cg->delta = sbsm->interval_ms / 1000.0;

	sbsm_rollback_players(cg, gss);
	sbsm_rollback_bullets(cg, gss);

	sbsm_rewind(cg, gss);
}

static inline void
sbsm_player_states_backfilling(cg_game_snapshot_t* new_gss, cg_game_snapshot_t* old_gss)
{
	ght_t* old_player_states = &old_gss->player_states;
	ght_t* new_player_states = &new_gss->player_states;

	GHT_FOREACH(cg_player_snapshot_t* pss, old_player_states, 
	{
		cg_player_snapshot_t* new_pss = ght_get(new_player_states, pss->player_id);
		if (new_pss == NULL)
		{
			cg_player_snapshot_t* pss_copy = malloc(sizeof(cg_player_snapshot_t));
			memcpy(pss_copy, pss, sizeof(cg_player_snapshot_t));
			ght_insert(new_player_states, pss_copy->player_id, pss_copy);
		}
	});
}

static inline void
sbsm_do_bullet_backfilling(coregame_t* cg, ght_t* new_bullet_states, cg_bullet_snapshot_t* old_bss)
{
	if (old_bss->collided)
	{
		/**
		 *	Do bullet deallaction here.
		 *	Only if bullet has collided.
		 */
		cg_bullet_t* bullet = ght_get(&cg->bullets, old_bss->bullet_id);
		if (bullet)
			coregame_free_bullet(cg, bullet);
		return;
	}

	cg_bullet_snapshot_t* new_bss = ght_get(new_bullet_states, old_bss->bullet_id);
	if (new_bss == NULL)
	{
		cg_bullet_snapshot_t* bss_copy = malloc(sizeof(cg_bullet_snapshot_t));
		memcpy(bss_copy, old_bss, sizeof(cg_bullet_snapshot_t));
		ght_insert(new_bullet_states, bss_copy->bullet_id, bss_copy);
	}
}

static inline void
sbsm_bullet_states_backfilling(coregame_t* cg, cg_game_snapshot_t* new_gss, cg_game_snapshot_t* old_gss)
{
	ght_t* old_bullet_states = &old_gss->bullet_states;
	ght_t* new_bullet_states = &new_gss->bullet_states;

	GHT_FOREACH(cg_bullet_snapshot_t* old_bss, old_bullet_states, 
	{
		sbsm_do_bullet_backfilling(cg, new_bullet_states, old_bss);
	});
}

static inline void
sbsm_state_backfilling(coregame_t* cg, cg_game_snapshot_t* new_gss, cg_game_snapshot_t* old_gss)
{
	sbsm_player_states_backfilling(new_gss, old_gss);
	sbsm_bullet_states_backfilling(cg, new_gss, old_gss);
}

void
sbsm_rotate(coregame_t* cg, cg_sbsm_t* sbsm)
{
	sbsm->present_idx++;
	if (sbsm->present_idx >= sbsm->size)
		sbsm->present_idx = 0;
	sbsm->present = sbsm->snapshots + sbsm->present_idx;

	if (sbsm->present == sbsm->base)
	{
		cg_game_snapshot_t* current_base_ss = sbsm->base;
		sbsm->base_idx++;
		if (sbsm->base_idx >= sbsm->size)
			sbsm->base_idx = 0;
		sbsm->base = sbsm->snapshots + sbsm->base_idx;

		sbsm_state_backfilling(cg, sbsm->base, current_base_ss);
	}

	sbsm->time += sbsm->interval_ms;
	sbsm->present->timestamp = sbsm->time;
	sbsm->present->seq = sbsm->seq++;
	ght_clear(&sbsm->present->player_states);
}

void
sbsm_print(const cg_sbsm_t* sbsm)
{
	printf("base time: %f ms., current_time: %f, interval: %f ms\n", 
			sbsm->base->timestamp, sbsm->present->timestamp, sbsm->interval_ms);

	u32 index = sbsm_index(sbsm, sbsm->base->timestamp);

	for (u32 i = 0; i < sbsm->size; i++)
	{
		cg_game_snapshot_t* ss = (cg_game_snapshot_t*)sbsm->snapshots + index;
		printf("%u ", ss->seq);
		if (ss == sbsm->present)
			printf(">");
		if (ss == sbsm->base)
			printf("|");
		if (ss->dirty)
		{
			printf("=");
			ss->dirty = false;
		}
		printf("\ttime: %f\n", ss->timestamp);
		ght_t* player_states = &ss->player_states;
		
		GHT_FOREACH(cg_player_snapshot_t* pss, player_states, {
			printf("\tp%u: pos: (%f/%f), input: ",
				pss->player_id, pss->pos.x, pss->pos.y);

			if (pss->input & PLAYER_INPUT_LEFT)
				printf("LEFT ");
			if (pss->input & PLAYER_INPUT_RIGHT)
				printf("RIGHT ");
			if (pss->input & PLAYER_INPUT_UP)
				printf("UP ");
			if (pss->input & PLAYER_INPUT_DOWN)
				printf("DOWN ");
			printf("(%u)\n", pss->input);
		});
		printf("\n");

		index++;
		if (index >= sbsm->size)
			index = 0;
	}
}

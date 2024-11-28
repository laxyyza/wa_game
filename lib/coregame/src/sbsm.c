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
		ght_init(&ss->bullet_states, 10, free);
	}

	sbsm_add_ss(NULL, sbsm);

	return sbsm;
}

static inline u32
sbsm_index(const cg_sbsm_t* sbsm, f64 timestamp_ms)
{
	return (u32)floor(timestamp_ms / sbsm->interval_ms) % sbsm->size;
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
sbsm_copy_player(cg_player_snapshot_t* pss, const cg_player_t* player)
{
	pss->player_id = player->id;
	pss->pos = player->pos;
	pss->input = player->input;
	pss->ammo = player->gun->ammo;
	pss->bullet_timer = player->gun->bullet_timer;
	pss->charge_timer = player->gun->charge_time;
	pss->reload_timer = player->gun->reload_time;
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
		ght_insert(&ss->player_states, pss->player_id, pss);
	}

	sbsm_copy_player(pss, player);
	pss->dirty = false;

	player->dirty = false;
}

void 
sbsm_commit_bullet(cg_game_snapshot_t* ss, cg_bullet_t* bullet)
{
	cg_bullet_snapshot_t* bss;

	bss = ght_get(&ss->bullet_states, bullet->id);

	if (bss == NULL)
	{
		bss = malloc(sizeof(cg_bullet_snapshot_t));
		bss->bullet_id = bullet->id;
		ght_insert(&ss->bullet_states, bss->bullet_id, bss);
	}
	bss->pos = bullet->r.pos;
	bss->collided = bullet->collided;
}

static void
sbsm_rewind_player(coregame_t* cg, cg_game_snapshot_t* ss, cg_player_t* player)
{
	cg_player_snapshot_t* pss = ght_get(&ss->player_states, player->id);

	if (pss)
	{
		if (pss->dirty)
		{
			if (pss->dirty_movement)
				coregame_set_player_input(player, pss->input);
			pss->dirty = false;
		}

		if (pss->dirty_fire)
			coregame_gun_update(cg, player->gun);
		if (pss->dirty_movement)
		{
			player->velocity.x = player->dir.x * PLAYER_SPEED;
			player->velocity.y = player->dir.y * PLAYER_SPEED;

			coregame_update_player(cg, player);
		}
	}

	sbsm_commit_player(ss, player);
}

static void 
sbsm_rewind(coregame_t* cg, cg_game_snapshot_t* ss)
{
	cg_sbsm_t* sbsm = cg->sbsm;
	u32 index = sbsm_index(sbsm, ss->timestamp);
	ght_t* players = &cg->players;

	cg->rewinding = true;

	while (ss->timestamp <= sbsm->present->timestamp)
	{
		if (ss == sbsm->present)
			break;

		GHT_FOREACH(cg_player_t* player, players, 
		{
			sbsm_rewind_player(cg, ss, player);
		});

		index++;
		if (index >= sbsm->size)
			index = 0;
		ss = sbsm->snapshots + index;
	}

	cg->rewinding = false;
}

static inline void
sbsm_rollback_player(coregame_t* cg, cg_player_snapshot_t* pss, ght_t* player_states)
{
	cg_player_t* player = NULL;

	player = ght_get(&cg->players, pss->player_id);
	if (player)
	{
		player->pos = pss->pos;
		player->gun->ammo = pss->ammo;
		player->gun->bullet_timer = pss->bullet_timer;
		player->gun->charge_time = pss->charge_timer;
		player->gun->reload_time = pss->reload_timer;
		coregame_set_player_input(player, pss->input);
		pss->dirty = false;
	}
	else
		ght_del(player_states, pss->player_id);
}

void 
sbsm_rollback(coregame_t* cg)
{
	cg_sbsm_t* sbsm = cg->sbsm;
	cg_game_snapshot_t* ss = sbsm->oldest_change;

	sbsm->oldest_change = NULL;

	cg->delta = sbsm->interval_ms / 1000.0;
	ght_t* player_states = &ss->player_states;

	GHT_FOREACH(cg_player_snapshot_t* pss, player_states, 
	{
		sbsm_rollback_player(cg, pss, player_states);
	});

	ght_t* bullet_states = &ss->bullet_states;
	GHT_FOREACH(cg_bullet_snapshot_t* bss, bullet_states, 
	{
		cg_bullet_t* bullet = ght_get(&cg->bullets, bss->bullet_id);
		if (bullet)
		{
			bullet->r.pos = bss->pos;
			bullet->collided = bss->collided;
		}
		else
			ght_del(bullet_states, bss->bullet_id);
	});

	sbsm_rewind(cg, ss);
}

void
sbsm_add_ss(coregame_t* cg, cg_sbsm_t* sbsm)
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

		ght_t* old_player_states = &current_base_ss->player_states;
		ght_t* new_player_states = &sbsm->base->player_states;

		ght_t* old_base_bullet_states = &current_base_ss->bullet_states;
		ght_t* new_base_bullet_states = &sbsm->base->bullet_states;

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

		/* Here is where bullets are deallcated at the end */
		GHT_FOREACH(cg_bullet_snapshot_t* old_base_bss, old_base_bullet_states, 
		{
			cg_bullet_t* bullet = ght_get(&cg->bullets, old_base_bss->bullet_id);
			if (bullet)
			{
				if (bullet->collided)
					coregame_free_bullet(cg, bullet);
				else if (ght_get(new_base_bullet_states, old_base_bss->bullet_id) == NULL)
				{
					cg_bullet_snapshot_t* bss_copy = malloc(sizeof(cg_bullet_snapshot_t));
					memcpy(bss_copy, old_base_bss, sizeof(cg_bullet_snapshot_t));
					ght_insert(new_base_bullet_states, bss_copy->bullet_id, bss_copy);
				}
			}
		});
	}

	sbsm->time += sbsm->interval_ms;
	sbsm->present->timestamp = sbsm->time;
	sbsm->present->seq = sbsm->seq++;
	ght_clear(&sbsm->present->player_states);
	ght_clear(&sbsm->present->bullet_states);
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
			printf("(%u), ", pss->input);

			printf("ammo: %d\n", pss->ammo);
		});
		printf("\n");

		index++;
		if (index >= sbsm->size)
			index = 0;
	}
}

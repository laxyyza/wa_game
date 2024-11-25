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
		ght_init(&ss->deltas, 10, free);
	}

	sbsm_add_ss(sbsm);

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
sbsm_commit_player(cg_game_snapshot_t* ss, cg_player_t* player)
{
	cg_player_snapshot_t* pss;

	if (player->dirty == false)
	{
		ght_del(&ss->deltas, player->id);
		return;
	}

	pss = ght_get(&ss->deltas, player->id);

	if (pss == NULL)
	{
		pss = malloc(sizeof(cg_player_snapshot_t));
		pss->player_id = player->id;
		pss->prev = NULL;
		ght_insert(&ss->deltas, pss->player_id, pss);
	}
	
	pss->pos = player->pos;
	pss->input = player->input;
	pss->dirty = false;
	player->dirty = false;
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
		// printf("Rewinding %f ms (%u)\n", ss->timestamp, ss->seq);

		GHT_FOREACH(cg_player_t* player, players, {
			cg_player_snapshot_t* pss = ght_get(&ss->deltas, player->id);

			if (pss && pss->dirty)
			{
				// printf("PSS:%u at ss:%u dirty.\n",
				// 		pss->player_id, ss->seq);
				coregame_set_player_input(player, pss->input);
				pss->dirty = false;
			}

			player->velocity.x = player->dir.x * PLAYER_SPEED;
			player->velocity.y = player->dir.y * PLAYER_SPEED;

			coregame_update_player(cg, player);

			sbsm_commit_player(ss, player);
		});

		if (ss == sbsm->present)
			break;

		index++;
		if (index >= sbsm->size)
			index = 0;
		ss = sbsm->snapshots + index;
	}

	cg->rewinding = false;
}

void 
sbsm_rollback(coregame_t* cg)
{
	cg_sbsm_t* sbsm = cg->sbsm;
	cg_game_snapshot_t* ss = sbsm->oldest_change;

	sbsm->oldest_change = NULL;

	cg->delta = sbsm->interval_ms / 1000.0;

	// printf("Rollback to %f ms (seq: %u)\n", ss->timestamp, ss->seq);

	ght_t* deltas = &ss->deltas;
	GHT_FOREACH(cg_player_snapshot_t* pss, deltas, {
		cg_player_t* player = ght_get(&cg->players, pss->player_id);
		if (player)
		{
			// printf("Player pos (%f/%f) -> (%f/%f)\n",
			// 	player->pos.x, player->pos.y,
			// 	pss->pos.x, pss->pos.y);
			player->pos = pss->pos;
			coregame_set_player_input(player, pss->input);
			pss->dirty = false;
		}
	});

	sbsm_rewind(cg, ss);
}

void
sbsm_add_ss(cg_sbsm_t* sbsm)
{
	sbsm->present_idx++;
	if (sbsm->present_idx >= sbsm->size)
		sbsm->present_idx = 0;
	sbsm->present = sbsm->snapshots + sbsm->present_idx;

	if (sbsm->present == sbsm->base)
	{
		// cg_game_snapshot_t* current_base_ss = sbsm->base;
		sbsm->base_idx++;
		if (sbsm->base_idx >= sbsm->size)
			sbsm->base_idx = 0;
		sbsm->base = sbsm->snapshots + sbsm->base_idx;
		//
		// ght_t* old_base_delta = &current_base_ss->deltas;
		// ght_t* new_base_delta = &sbsm->base->deltas;
		//
		// GHT_FOREACH(cg_player_snapshot_t* pss, old_base_delta, {
		// 	cg_player_snapshot_t* new_pss = ght_get(new_base_delta, pss->player_id);
		// 	if (new_pss == NULL)
		// 	{
		// 		cg_player_snapshot_t* pss_copy = malloc(sizeof(cg_player_snapshot_t));
		// 		memcpy(pss_copy, pss, sizeof(cg_player_snapshot_t));
		// 		ght_insert(new_base_delta, pss_copy->player_id, pss_copy);
		// 	}
		// });
	}

	sbsm->time += sbsm->interval_ms;
	sbsm->present->timestamp = sbsm->time;
	sbsm->present->seq = sbsm->seq++;
	ght_clear(&sbsm->present->deltas);
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
		ght_t* deltas = &ss->deltas;
		
		GHT_FOREACH(cg_player_snapshot_t* pss, deltas, {
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

#ifndef _SBSM_H_
#define _SBSM_H_

/**
 *	SBSM - Snapshot-Based Sate Management 
 */

#include <int.h>
#include <array.h>
#include <ght.h>
#include "vec.h"

typedef struct cg_game_snapshot cg_game_snapshot_t;
typedef struct coregame coregame_t;
typedef struct cg_player cg_player_t;

typedef struct 
{
	u32		player_id;
	vec2f_t pos;
	u8		input;
	bool	dirty;
} cg_player_snapshot_t;

typedef struct cg_game_snapshot
{
	f64 timestamp;
	u32 seq;
	ght_t deltas;
	bool dirty;
} cg_game_snapshot_t;

typedef struct 
{
	u32 seq;
	f64 interval_ms;
	f64 time;
	u32 size;
	bool dirty;
	
	cg_game_snapshot_t* oldest_change;

	cg_game_snapshot_t* base;
	u32 base_idx;

	cg_game_snapshot_t* present;
	u32 present_idx;

	cg_game_snapshot_t snapshots[];
} cg_sbsm_t;	// Snapshot-Based State Management

cg_sbsm_t* sbsm_create(u32 count, f64 interval_ms);
cg_game_snapshot_t* sbsm_lookup(cg_sbsm_t* sbsm, f64 timestamp_ms);
void sbsm_rollback(coregame_t* cg);
void sbsm_add_ss(cg_sbsm_t* sbsm);
void sbsm_print(const cg_sbsm_t* sbsm);
void sbsm_commit_player(cg_game_snapshot_t* ss, cg_player_t* player);

#endif // _SBSM_H_

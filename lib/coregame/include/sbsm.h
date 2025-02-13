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
typedef struct cg_bullet cg_bullet_t;

typedef struct 
{
	u32		player_id;
	vec2f_t pos;
	vec2f_t velocity;
	u8		input;
	bool	shooting;

	i32 ammo;
	f32 bullet_timer;
	f32 charge_timer;
	f32 reload_timer;
	u32 gun_id;

	u32		dirty_count;
	bool	dirty;
	bool	dirty_move;
	bool	dirty_gun;
} cg_player_snapshot_t;

typedef struct 
{
	u32 bullet_id;
	vec2f_t pos;
	bool collided;
} cg_bullet_snapshot_t;

typedef struct cg_game_snapshot
{
	f64 timestamp;
	u32 seq;
	ght_t player_states;
	ght_t bullet_states;
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
void sbsm_rotate(coregame_t* cg, cg_sbsm_t* sbsm);
void sbsm_print(const cg_sbsm_t* sbsm);
void sbsm_commit_player(cg_game_snapshot_t* ss, cg_player_t* player);
void sbsm_commit_bullet(cg_game_snapshot_t* ss, cg_bullet_t* bullet);
void sbsm_player_to_snapshot(cg_player_snapshot_t* pss, const cg_player_t* player);
void sbsm_snapshot_to_player(coregame_t* cg, cg_player_t* player, const cg_player_snapshot_t* pss);
void sbsm_snapshot_to_bullet(cg_bullet_t* bullet, const cg_bullet_snapshot_t* bss);
void sbsm_delete_player(cg_sbsm_t* sbsm, cg_player_t* player);

#endif // _SBSM_H_

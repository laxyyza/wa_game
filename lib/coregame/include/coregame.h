#ifndef _CORE_GAME_H_
#define _CORE_GAME_H_

#define _GNU_SOURCE
#include <int.h>
#include <ght.h>
#include "rect.h"
#include <time.h>
#include "cg_map.h"
#include "mmframes.h"

#define INTERPOLATE_FACTOR			0.2
#define INTERPOLATE_THRESHOLD_DIST	0.001

#define PROJ_SPEED	  7000
#define PLAYER_SPEED  1400
#define PLAYER_HEALTH 100
#define	PROJ_DMG	  10
#define PLAYER_NAME_MAX 32
#define UNUSED __attribute__((unused))

#define PLAYER_DIR_UP	 0x01
#define PLAYER_DIR_DOWN	 0x02
#define PLAYER_DIR_RIGHT 0x04
#define PLAYER_DIR_LEFT  0x08

typedef struct cg_player cg_player_t;

typedef void (*cg_player_changed_callback_t)(cg_player_t* player, void* user_data);
typedef void (*cg_player_damaged_callback_t)(cg_player_t* target_player, 
											 cg_player_t* attacker_player, void* user_data);

typedef struct 
{
	u16 kills;
	u16 deaths;
	f32 ping;
} cg_player_stats_t;

typedef struct cg_player
{
	u32		id;
	vec2f_t pos;
	vec2f_t dir;
	i32		health;
	i32		max_health;

	vec2f_t size;
	vec2f_t prev_pos;
	vec2f_t prev_dir;
	vec2f_t cursor;
	char	username[PLAYER_NAME_MAX];

	struct {
		vec2f_t server_pos;
		bool	interpolate;
	};
	cg_player_stats_t stats;
} cg_player_t;

typedef struct cg_projectile
{
	u32 owner;
	cg_rect_t rect;
	vec2f_t dir;
	vec2f_t prev_pos;
	f32 rotation;

	struct cg_projectile* next;
	struct cg_projectile* prev;
} cg_projectile_t;

typedef struct coregame 
{
	ght_t players;
	struct {
		cg_projectile_t* head;
		cg_projectile_t* tail;
	} proj;
	cg_rect_t world_border;
	cg_map_t* map;

	struct timespec last_time;
	f64 delta;
	void* user_data;

	void (*proj_free_callback)(cg_projectile_t* proj, void* data);
	void (*player_free_callback)(cg_player_t* proj, void* data);
	cg_player_changed_callback_t player_changed;
	cg_player_damaged_callback_t player_damaged;

	f32 interp_factor;
	f32 interp_threshold_dist;
	f32 time_scale;

	bool client;
} coregame_t;

void coregame_init(coregame_t* coregame, bool client, cg_map_t* map);
void coregame_update(coregame_t* coregame);
void coregame_cleanup(coregame_t* coregame);

cg_player_t* coregame_add_player(coregame_t* coregame, const char* name);
void coregame_add_player_from(coregame_t* coregame, cg_player_t* player);
void coregame_free_player(coregame_t* coregame, cg_player_t* player);
void coregame_set_player_dir(cg_player_t* player, u8 dir);
cg_projectile_t* coregame_player_shoot(coregame_t* coregame, cg_player_t* player, vec2f_t dir);

cg_projectile_t* coregame_add_projectile(coregame_t* coregame, cg_player_t* player);
void coregame_free_projectile(coregame_t* coregame, cg_projectile_t* proj);

f32  coregame_dist(const vec2f_t* a, const vec2f_t* b);
void coregame_randb(void* buf, u64 count);

#endif // _CORE_GAME_H_

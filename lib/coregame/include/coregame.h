#ifndef _CORE_GAME_H_
#define _CORE_GAME_H_

#define _GNU_SOURCE
#include <int.h>
#include <ght.h>
#include "rect.h"
#include "cg_map.h"
#include "mmframes.h"
#include "nano_timer.h"

#ifdef CG_SERVER
#include "sbsm.h"
#endif

#define INTERPOLATE_FACTOR			0.01
#define INTERPOLATE_THRESHOLD_DIST	10.0
#define INTERPOLATE_THRESHOLD_DIST_LOW	1.0

#define GUN_BPS 20.0
#define BULLET_SPEED  7000
#define PLAYER_SPEED  1400
#define PLAYER_HEALTH 100
#define	BULLET_DMG	  2.5
#define PLAYER_NAME_MAX 32

#define UNUSED __attribute__((unused))
#ifdef _WIN32
#define UNUSED_WIN32 UNUSED
#else
#define UNUSED_WIN32
#endif 

#define PLAYER_INPUT_UP	   0x01
#define PLAYER_INPUT_DOWN  0x02
#define PLAYER_INPUT_RIGHT 0x04
#define PLAYER_INPUT_LEFT  0x08

#define PLAYER_MOVE_INPUT  0x0F
#define PLAYER_GUN_INPUT   0x10

#define PLAYER_INPUT_SHOOT 0x10

typedef struct cg_player cg_player_t;
typedef struct cg_bullet cg_bullet_t;
typedef struct coregame coregame_t;
typedef struct cg_gun cg_gun_t;

typedef void (*cg_bullet_create_callback_t)(cg_bullet_t* bullet, void* user_data);
typedef void (*cg_player_changed_callback_t)(cg_player_t* player, void* user_data);
typedef void (*cg_player_reload_callback_t)(cg_player_t* player, void* user_data);
typedef void (*cg_player_damaged_callback_t)(cg_player_t* target_player, 
											 cg_player_t* attacker_player, void* user_data);
typedef void (*cg_player_free_callback_t)(cg_player_t* player);

enum cg_gun_id
{
	CG_GUN_ID_SMALL,
	CG_GUN_ID_BIG,
	CG_GUN_ID_MINI_GUN,

	CG_GUN_ID_TOTAL
};

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
	vec2f_t velocity;
	f32		health;
	f32		max_health;
	cg_gun_t* gun;

	vec2f_t size;
	vec2f_t prev_pos;
	vec2f_t prev_dir;
	vec2f_t cursor;
	char	username[PLAYER_NAME_MAX];
	array_t cells;
	u8		input;

#ifdef CG_SERVER
	bool	dirty;
	bool	gun_dirty;
	f64		last_input_timestamp;
#endif

#ifdef CG_CLIENT
	vec2f_t server_pos;
	bool	interpolate;
	bool	bad_local_pos;
#endif

	cg_player_stats_t stats;
	void*	user_data;
	bool	shoot;
	cg_player_free_callback_t on_player_free;
} cg_player_t;

typedef struct cg_bullet
{
	u32				id;
	u32				owner_id;
	cg_rect_t		r;
	vec2f_t			dir;
	vec2f_t			velocity;
	f32				dmg;
	enum cg_gun_id	gun_id;
	array_t			cells;
	void*			data;
	bool			collided;
	vec2f_t			contact_point;

	struct cg_bullet* next;
	struct cg_bullet* prev;
} cg_bullet_t;

typedef struct cg_gun_spec
{
	enum cg_gun_id id;
	f32 bps;
	f32 initial_charge_time;
	f32 bullet_spawn_interval;
	f32 bullet_speed;
	f32 dmg;
	f32 knockback_force;
	i32 max_ammo;
	f32 reload_time;
	bool autocharge;
} cg_gun_spec_t;

typedef struct cg_gun
{
	const cg_gun_spec_t* spec;
	f32 bullet_timer;
	f32 charge_time;
	i32 ammo;
	f32 reload_time;
	cg_player_t* owner;

	/**	`shoot`
	 *	The creation of a new bullet.
	 */
	void (*shoot)(coregame_t* cg, struct cg_gun* gun);

	/**	`data[]`
	 *	I'm think of having multiple different guns.
	 *	So they might have gun-specific data, it will here in `u8 data[]`.
	 */
	u8 data[];
} cg_gun_t;

typedef struct coregame 
{
	ght_t players;
	ght_t bullets;
	cg_rect_t world_border;
	cg_runtime_map_t* map;

	array_t gun_specs;
	hr_time_t last_time;
	f64 delta;
	void* user_data;

	cg_bullet_create_callback_t on_bullet_create;
	void (*bullet_free_callback)(cg_bullet_t* bullet, void* data);
	cg_player_free_callback_t    player_free_callback;	
	cg_player_reload_callback_t  player_reload;
	cg_player_changed_callback_t player_changed;
	cg_player_changed_callback_t player_gun_changed;
	cg_player_damaged_callback_t player_damaged;

	f32 time_scale;
	u32 player_id_seq;
	u32 bullet_id_seq;

	bool pause;

#ifdef CG_SERVER
	cg_sbsm_t* sbsm;
	bool rewinding;
#endif // CG_SERVER

#ifdef CG_CLIENT
	f32 local_interp_factor;
	f32 target_local_interp_factor;
	f32 remote_interp_factor;
	f32 target_remote_interp_factor;

	f32 interp_threshold_dist;

	cg_player_t* local_player;
#endif // CG_CLIENT
} coregame_t;

void coregame_init(coregame_t* coregame, cg_runtime_map_t* map);
void coregame_server_init(coregame_t* cg, cg_runtime_map_t* map, f32 tick_per_sec);
void coregame_update(coregame_t* coregame);
void coregame_cleanup(coregame_t* coregame);

cg_player_t* coregame_add_player(coregame_t* coregame, const char* name);
void coregame_add_player_from(coregame_t* coregame, cg_player_t* player);
void coregame_free_player(coregame_t* coregame, cg_player_t* player);
void coregame_set_player_input(cg_player_t* player, u8 input);

#ifdef CG_SERVER
	void coregame_set_player_input_t(coregame_t* cg, cg_player_t* player, u8 input, f64 timestamp);
#endif 

u8	 coregame_get_player_input(const cg_player_t* player);
void coregame_free_bullet(coregame_t* coregame, cg_bullet_t* bullet);

f32  coregame_dist(const vec2f_t* a, const vec2f_t* b);

cg_gun_t* coregame_create_gun(coregame_t* cg, enum cg_gun_id id, cg_player_t* owner);
void coregame_add_gun_spec(coregame_t* cg, const cg_gun_spec_t* spec);
void coregame_gun_update(coregame_t* cg, cg_gun_t* gun);
bool coregame_player_change_gun_force(coregame_t* cg, cg_player_t *player, enum cg_gun_id id);
bool coregame_player_change_gun(coregame_t* cg, cg_player_t* player, enum cg_gun_id id);
void coregame_player_reload(coregame_t* cg, cg_player_t* player);
void coregame_update_player(coregame_t* coregame, cg_player_t* player);
void coregame_update_bullet(coregame_t* cg, cg_bullet_t* bullet);
cg_bullet_t* cg_add_bullet(coregame_t* cg, cg_gun_t* gun);

#endif // _CORE_GAME_H_

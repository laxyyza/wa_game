#ifndef _CORE_GAME_H_
#define _CORE_GAME_H_

#include <int.h>
#include <ght.h>
#include "rect.h"

#define PLAYER_SPEED  10
#define PLAYER_HEALTH 100
#define	PROJ_DMG	  40
#define PLAYER_NAME_MAX 32
#define UNUSED __attribute__((unused))

#define PLAYER_DIR_UP	 0x01
#define PLAYER_DIR_DOWN	 0x02
#define PLAYER_DIR_RIGHT 0x04
#define PLAYER_DIR_LEFT  0x08

typedef struct 
{
	u32 id;
	char username[PLAYER_NAME_MAX];
	cg_rect_t rect;
	i32 health;
	vec2f_t dir;
} cg_player_t;

typedef struct 
{
	u32 id;
	u32 owner;
	cg_rect_t rect;
	vec2f_t dir;
} cg_projectile_t;

typedef struct coregame 
{
	ght_t players;
	ght_t projectiles;
	cg_rect_t world_border;
} coregame_t;

void coregame_init(coregame_t* coregame);
void coregame_update(coregame_t* coregame);
void coregame_cleanup(coregame_t* coregame);

cg_player_t* coregame_add_player(coregame_t* coregame, const char* name);
void coregame_free_player(coregame_t* coregame, cg_player_t* player);
void coregame_set_player_dir(cg_player_t* player, u8 dir);

cg_projectile_t* coregame_add_projectile(coregame_t* coregame, cg_player_t* player);
void coregame_free_projectile(coregame_t* coregame, cg_projectile_t* proj);

#endif // _CORE_GAME_H_

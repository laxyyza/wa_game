#ifndef _CLIENT_PLAYER_H_
#define _CLIENT_PLAYER_H_

#include "coregame.h"
#include "rect.h"

typedef struct waapp waapp_t;

typedef struct 
{	
	rect_t background;
	rect_t fill;
	f32 fill_width;
} healthbar_t;

typedef struct 
{
	cg_player_t* core;
	rect_t rect;
	rect_t top;
	u8 movement_dir;
	healthbar_t hpbar; 
} player_t;

typedef struct 
{
	char target_name[PLAYER_NAME_MAX];
	char attacker_name[PLAYER_NAME_MAX];
	struct timespec timestamp;
} player_kill_t;

typedef struct 
{
	cg_projectile_t* core;
	rect_t rect;
} projectile_t;

player_t* player_new(waapp_t* app, const char* name);
player_t* player_new_from(waapp_t* app, cg_player_t* cg_player);
void	  player_set_health(player_t* player, i32 new_hp);

#endif // _CLIENT_PLAYER_H_

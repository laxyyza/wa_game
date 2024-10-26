#ifndef _CLIENT_PLAYER_H_
#define _CLIENT_PLAYER_H_

#include "coregame.h"
#include "rect.h"

typedef struct waapp waapp_t;

typedef struct 
{
	cg_player_t* core;
	rect_t rect;
	rect_t top;
	u8 movement_dir;
} player_t;

typedef struct 
{
	cg_projectile_t* core;
	rect_t rect;
} projectile_t;

player_t* player_new(waapp_t* app, const char* name);
projectile_t* projectile_new(waapp_t* app, cg_projectile_t* core_proj);

#endif // _CLIENT_PLAYER_H_

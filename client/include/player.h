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

player_t* player_new(waapp_t* app, const char* name);

#endif // _CLIENT_PLAYER_H_

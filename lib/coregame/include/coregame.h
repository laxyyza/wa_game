#ifndef _CORE_GAME_H_
#define _CORE_GAME_H_

#include "int.h"

#define PLAYER_NAME_MAX 32

typedef struct 
{
	u32 id;
	char username[PLAYER_NAME_MAX];
	rect_t rect;
	i32 health;
} cg_player_t;

typedef struct coregame 
{
} coregame_t;

#endif // _CORE_GAME_H_

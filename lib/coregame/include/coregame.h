#ifndef _CORE_GAME_H_
#define _CORE_GAME_H_

#include "int.h"

#define PLAYER_NAME_MAX 32
#define UNUSED __attribute__((unused))

typedef struct 
{
	u32 id;
	char username[PLAYER_NAME_MAX];
	i32 health;
} cg_player_t;

typedef struct coregame 
{
} coregame_t;

#endif // _CORE_GAME_H_

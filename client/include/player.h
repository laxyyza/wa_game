#ifndef _CLIENT_PLAYER_H_
#define _CLIENT_PLAYER_H_

#include "coregame.h"
#include "rect.h"
#include "progress_bar.h"

typedef struct waapp waapp_t;
typedef struct client_game client_game_t;

typedef struct 
{
	cg_player_t* core;
	rect_t rect;
	rect_t gun_rect;
	u8 input;
	progress_bar_t hpbar; 
	progress_bar_t guncharge; 
} player_t;

typedef struct 
{
	char target_name[PLAYER_NAME_MAX];
	char attacker_name[PLAYER_NAME_MAX];
	struct timespec timestamp;
} player_kill_t;

player_t* player_new(client_game_t* game, const char* name);
player_t* player_new_from(client_game_t* game, cg_player_t* cg_player);
void player_update_guncharge(player_t* player, progress_bar_t* bar);

#endif // _CLIENT_PLAYER_H_

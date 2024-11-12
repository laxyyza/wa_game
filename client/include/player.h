#ifndef _CLIENT_PLAYER_H_
#define _CLIENT_PLAYER_H_

#include "coregame.h"
#include "rect.h"

typedef struct waapp waapp_t;
typedef struct client_game client_game_t;

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

player_t* player_new(client_game_t* game, const char* name);
player_t* player_new_from(client_game_t* game, cg_player_t* cg_player);
void	  player_set_health(player_t* player, i32 new_hp);

#endif // _CLIENT_PLAYER_H_

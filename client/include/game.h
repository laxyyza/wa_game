#ifndef _CLIENT_GAME_H_
#define _CLIENT_GAME_H_

#include "coregame.h"
#include "player.h"
#include "wa.h"
#include "renderer.h"
#include "client_net.h"

typedef struct waapp waapp_t;

typedef struct client_game
{
	coregame_t cg;
	ght_t players;
	player_t* player;
	vec2f_t prev_dir;
	vec2f_t prev_pos;

	texture_t* tank_bottom_tex;
	texture_t* tank_top_tex;

	bool	lock_cam;
	bool	trigger_shooting;

	array_t player_deaths;

	f64 death_kill_time;

	ren_t* ren;
	waapp_t* app;
	client_net_t* net;

	struct nk_context* nk_ctx;
} client_game_t;

void* game_init(waapp_t* app);
void game_update(waapp_t* app, client_game_t* game);
i32  game_event(waapp_t* app, const wa_event_t* ev);
void game_cleanup(waapp_t* app, client_game_t* game);
void game_handle_mouse_wheel(waapp_t* app, const wa_event_wheel_t* ev);
void game_move_cam(waapp_t* app);
void game_lock_cam(client_game_t* game);

#endif // _CLIENT_GAME_H_


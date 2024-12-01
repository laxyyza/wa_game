#ifndef _CLIENT_GAME_H_
#define _CLIENT_GAME_H_

#include "coregame.h"
#include "player.h"
#include "wa.h"
#include "renderer.h"
#include "client_net.h"

#define UI_LABEL_SIZE 128

typedef struct waapp waapp_t;

typedef struct 
{
	char name[PLAYER_NAME_MAX];
	u32  name_len;
	char msg[CHAT_MSG_MAX];
	u32  msg_len;
} chatmsg_t;

typedef struct laser_bullet
{
	f32 len;
	f32 thickness;
	f32 thickness_scaled;
} laser_bullet_t;

typedef struct client_game
{
	coregame_t cg;
	player_t* player;
	vec2f_t prev_pos;

	texture_t* tank_bottom_tex;
	texture_t* gun_textures[CG_GUN_ID_TOTAL];

	bool	lock_cam;

	array_t player_deaths;

	f64 death_kill_time;

	ren_t* ren;
	waapp_t* app;
	client_net_t* net;
	bool open_chat;
	bool new_msg;
	bool game_debug;
	bool game_netdebug;
	bool ignore_server_pos;
	bool ignore_auto_interp;
	f64 last_chatmsg;
	array_t chat_msgs;
	bro_t* laser_bro;

	struct nk_context* nk_ctx;

	laser_bullet_t small_laser;
	laser_bullet_t big_laser;

	progress_bar_t health_bar;
	progress_bar_t guncharge_bar;

	u8 prev_input;

	char ui_label[UI_LABEL_SIZE];

	bool show_stats;
} client_game_t;

void* game_init(waapp_t* app);
void game_update(waapp_t* app, client_game_t* game);
i32  game_event(waapp_t* app, client_game_t* game, const wa_event_t* ev);
void game_cleanup(waapp_t* app, client_game_t* game);
void game_handle_mouse_wheel(waapp_t* app, const wa_event_wheel_t* ev);
void game_move_cam(waapp_t* app);
void game_lock_cam(client_game_t* game);
void game_add_chatmsg(client_game_t* game, const char* name, const char* msg);
void game_send_chatmsg(client_game_t* game, const char* msg);
void game_update_ui_bars_pos(client_game_t* game);

#endif // _CLIENT_GAME_H_


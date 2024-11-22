#define _GNU_SOURCE
#include "game_net_events.h"
#include "main_menu.h"
#include <time.h>

void 
game_new_player(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _)
{
	const net_tcp_new_player_t* new_player = (const net_tcp_new_player_t*)segment->data;
	client_game_t* game = app->game;

	cg_player_t* cg_player = calloc(1, sizeof(cg_player_t));
	cg_player->id = new_player->id;
	cg_player->pos = new_player->pos;
	cg_player->size = new_player->size;
	cg_player->dir = new_player->dir;
	cg_player->cursor = new_player->cursor;
	cg_player->health = new_player->health;
	cg_player->max_health = new_player->max_health;
	cg_player->shoot = new_player->shoot;
	strncpy(cg_player->username, new_player->username, PLAYER_NAME_MAX);

	coregame_create_gun(&app->game->cg, new_player->gun_id, cg_player);

	player_t* player = player_new_from(app->game, cg_player);

	char msg[CHAT_MSG_MAX];
	snprintf(msg, CHAT_MSG_MAX, "[%s entered the game]", cg_player->username);
	game_add_chatmsg(app->game, NULL, msg);

	if (cg_player->id == app->net.player_id)
	{
		app->game->player = player;
		player->hpbar.fill.color = rgba(0x00FF00FF);

		progress_bar_init(&game->health_bar, vec2f(0, 0), vec2f(250, 30), 
							&cg_player->max_health, &cg_player->health, 0);
		game->health_bar.fill.color = player->hpbar.fill.color;

		progress_bar_init(&game->guncharge_bar, vec2f(0, 0), vec2f(250, 20), 
							NULL, NULL, 0);
		game->guncharge_bar.fill.color = player->guncharge.fill.color;

		game_update_ui_bars_pos(game);
	}

	coregame_add_player_from(&app->game->cg, cg_player);
}

void 
game_delete_player(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _)
{
	const net_tcp_delete_player_t* del_player = (const net_tcp_delete_player_t*)segment->data;

	cg_player_t* player = ght_get(&app->game->cg.players, del_player->player_id);
	if (player == NULL)
		return;

	char msg[CHAT_MSG_MAX];
	snprintf(msg, CHAT_MSG_MAX, "[%s left the game]", player->username);
	game_add_chatmsg(app->game, NULL, msg);

	coregame_free_player(&app->game->cg, player);
}

void 
game_player_move(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _)
{
	const net_udp_player_move_t* move = (net_udp_player_move_t*)segment->data;
	const vec2f_t* server_pos = &move->pos;
	const vec2f_t* client_pos;
	cg_player_t* player = ght_get(&app->game->cg.players, move->player_id);

	if (player)
	{
		client_pos = &player->pos;

		f32 dist = coregame_dist(client_pos, server_pos);

		if (dist > app->game->cg.interp_threshold_dist)
		{
			player->interpolate = true;
			player->server_pos = *server_pos;
		}
		else
			player->pos = *server_pos;
		player->dir = move->dir;
	}
}

void 
game_player_cursor(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _)
{
	const net_udp_player_cursor_t* cursor = (net_udp_player_cursor_t*)segment->data;
	cg_player_t* player = ght_get(&app->game->cg.players, cursor->player_id);
	if (player)
	{
		player->cursor = cursor->cursor_pos;
	}
}

void 
game_player_health(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _)
{
	const net_udp_player_health_t* health = (net_udp_player_health_t*)segment->data;
	cg_player_t* cg_player = ght_get(&app->game->cg.players, health->player_id);
	if (cg_player)		
	{
		cg_player->health = health->health;
		player_t* player = cg_player->user_data;
		progress_bar_update(&player->hpbar);

		if (cg_player->id == app->game->player->core->id)
			progress_bar_update(&app->game->health_bar);
	}
}

void 
game_player_died(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _)
{
	const net_udp_player_died_t* died = (const net_udp_player_died_t*)segment->data;
	cg_player_t* target;
	cg_player_t* attacker;
	player_kill_t* kill;

	target = ght_get(&app->game->cg.players, died->target_player_id);
	if (target == NULL)
	{
		errorf("player_died: target player %u not found.\n",
			died->target_player_id);
		return;
	}
	attacker = ght_get(&app->game->cg.players, died->attacker_player_id);
	if (attacker == NULL)
	{
		errorf("player_died: attacker player %u not found.\n",
			died->attacker_player_id);
		return;
	}

	kill = array_add_into(&app->game->player_deaths);
	strncpy(kill->target_name, target->username, PLAYER_NAME_MAX);
	strncpy(kill->attacker_name, attacker->username, PLAYER_NAME_MAX);
	clock_gettime(CLOCK_MONOTONIC, &kill->timestamp);
}

void 
game_player_stats(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _)
{
	const net_udp_player_stats_t* stats = (const net_udp_player_stats_t*)segment->data;
	cg_player_t* player;

	player = ght_get(&app->game->cg.players, stats->player_id);
	if (player)
	{
		player->stats.kills = stats->kills;
		player->stats.deaths = stats->deaths;
	}
}

void 
game_player_ping(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _)
{
	const net_udp_player_ping_t* ping = (const net_udp_player_ping_t*)segment->data;
	cg_player_t* player;

	if ((player = ght_get(&app->game->cg.players, ping->player_id)))
		player->stats.ping = ping->ms;
}

void 
game_cg_map(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _)
{
	const net_tcp_cg_map_t* tcp_map = (const net_tcp_cg_map_t*)segment->data;

	app->map_from_server = cg_map_load_disk(tcp_map, segment->size);
}

void 
game_server_shutdown(UNUSED const ssp_segment_t* segment, waapp_t* app, UNUSED void* _)
{
	waapp_main_menu_t* mm = app->sm.states.main_menu.data;
	strcpy(mm->state, "Server shutdown");
	waapp_state_switch(app, &app->sm.states.main_menu);
}

void 
game_chat_msg(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _)
{
	cg_player_t* player;
	const net_tcp_chat_msg_t* chatmsg = (const void*)segment->data;
	const char* name = "[[unknown]]";

	if (chatmsg->player_id == 0)
		name = NULL;
	else if ((player = ght_get(&app->game->cg.players, chatmsg->player_id)))
		name = player->username;

	game_add_chatmsg(app->game, name, chatmsg->msg);
}

void 
game_gun_spec(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _)
{
	const cg_gun_spec_t* gun_spec = (const cg_gun_spec_t*)segment->data;

	if (app->game)
	{
		coregame_add_gun_spec(&app->game->cg, gun_spec);
	}
	else
	{
		cg_gun_spec_t* new_spec;

		if (app->tmp_gun_specs == NULL)
		{
			app->tmp_gun_specs = calloc(1, sizeof(array_t));
			array_init(app->tmp_gun_specs, sizeof(cg_gun_spec_t), 4);
		}

		new_spec = array_add_into(app->tmp_gun_specs);
		memcpy(new_spec, gun_spec, sizeof(cg_gun_spec_t));
	}
}

void 
game_player_gun_id(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _)
{
	const net_udp_player_gun_id_t* player_gun_id = (const net_udp_player_gun_id_t*)segment->data;
	cg_player_t* player = ght_get(&app->game->cg.players, player_gun_id->player_id);

	if (player)
		coregame_player_change_gun_force(&app->game->cg, player, player_gun_id->gun_id);
}

void 
game_player_input(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _)
{
	const net_udp_player_input_t* input = (const net_udp_player_input_t*)segment->data;
	cg_player_t* player = ght_get(&app->game->cg.players, input->player_id);

	if (player)
		coregame_set_player_input(player, input->flags);
}

void 
game_player_reload(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _)
{
	const net_udp_player_reload_t* reload = (const net_udp_player_reload_t*)segment->data;
	cg_player_t* player = ght_get(&app->game->cg.players, reload->player_id);

	if (player)
		coregame_player_reload(&app->game->cg, player);
}

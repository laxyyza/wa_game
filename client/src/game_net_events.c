#define _GNU_SOURCE
#include "game_net_events.h"
#include "main_menu.h"
#include <time.h>

void 
game_new_player(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _)
{
	const cg_player_t* new_player = (const cg_player_t*)segment->data;

	cg_player_t* cg_player = malloc(sizeof(cg_player_t));
	memcpy(cg_player, new_player, sizeof(cg_player_t));

	player_t* player = player_new_from(app->game, cg_player);

	if (cg_player->id == app->net.player_id)
	{
		app->game->player = player;
		player->hpbar.fill.color = rgba(0x00FF00FF);
	}

	coregame_add_player_from(&app->game->cg, cg_player);
}

void 
game_delete_player(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _)
{
	const net_tcp_delete_player_t* del_player = (const net_tcp_delete_player_t*)segment->data;

	cg_player_t* player = ght_get(&app->game->cg.players, del_player->player_id);
	if (player)
	{
		coregame_free_player(&app->game->cg, player);
	}
}

void 
game_player_move(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _)
{
	const net_udp_player_move_t* move = (net_udp_player_move_t*)segment->data;
	const vec2f_t* server_pos = &move->pos;
	const vec2f_t* client_pos;
	player_t* player = ght_get(&app->game->players, move->player_id);

	if (player)
	{
		client_pos = &player->core->pos;

		f32 dist = coregame_dist(client_pos, server_pos);

		if (dist > app->game->cg.interp_threshold_dist)
		{
			player->core->interpolate = true;
			player->core->server_pos = *server_pos;
		}
		else
			player->core->pos = *server_pos;
		player->core->dir = move->dir;
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
game_player_shoot(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _)
{
	const net_udp_player_shoot_t* shoot = (net_udp_player_shoot_t*)segment->data;
	cg_player_t* player = ght_get(&app->game->cg.players, shoot->player_id);
	if (player)
		coregame_player_shoot(&app->game->cg, player, shoot->shoot_dir);
}

void 
game_player_health(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _)
{
	const net_udp_player_health_t* health = (net_udp_player_health_t*)segment->data;
	player_t* player = ght_get(&app->game->players, health->player_id);
	if (player)
		player_set_health(player, health->health);
}

void 
game_player_died(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _)
{
	const net_udp_player_died_t* died = (const net_udp_player_died_t*)segment->data;
	player_t* target;
	player_t* attacker;
	player_kill_t* kill;

	target = ght_get(&app->game->players, died->target_player_id);
	if (target == NULL)
	{
		errorf("player_died: target player %u not found.\n",
			died->target_player_id);
		return;
	}
	attacker = ght_get(&app->game->players, died->attacker_player_id);
	if (attacker == NULL)
	{
		errorf("player_died: attacker player %u not found.\n",
			died->attacker_player_id);
		return;
	}

	kill = array_add_into(&app->game->player_deaths);
	strncpy(kill->target_name, target->core->username, PLAYER_NAME_MAX);
	strncpy(kill->attacker_name, attacker->core->username, PLAYER_NAME_MAX);
	clock_gettime(CLOCK_MONOTONIC, &kill->timestamp);
}

void 
game_player_stats(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _)
{
	const net_udp_player_stats_t* stats = (const net_udp_player_stats_t*)segment->data;
	player_t* player;

	player = ght_get(&app->game->players, stats->player_id);
	if (player)
	{
		player->core->stats.kills = stats->kills;
		player->core->stats.deaths = stats->deaths;
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

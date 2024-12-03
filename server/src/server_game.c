#include "server_game.h"

void
server_on_player_reload(cg_player_t* player, server_t* server)
{
	client_t* source_client = player->user_data;

	net_udp_player_reload_t* reload_out = mmframes_alloc(&server->mmf, sizeof(net_udp_player_reload_t));
	reload_out->player_id = source_client->player->id;

	server_add_data_all_udp_clients_i(server, NET_UDP_PLAYER_RELOAD, reload_out, sizeof(net_udp_player_reload_t), player->id);
}

static net_tcp_new_player_t*
client_to_tcp_new_player(server_t* server, const client_t* client)
{
	const cg_player_t* player = client->player;

	net_tcp_new_player_t* tcp_new_player = mmframes_alloc(&server->mmf, sizeof(net_tcp_new_player_t));
	tcp_new_player->id = player->id;
	tcp_new_player->gun_id = player->gun->spec->id;
	tcp_new_player->pos = player->pos;
	tcp_new_player->size = player->size;
	tcp_new_player->dir = player->dir;
	tcp_new_player->cursor = player->cursor;
	tcp_new_player->health = player->health;
	tcp_new_player->max_health = player->max_health;
	tcp_new_player->shoot = player->shoot;
	strncpy(tcp_new_player->username, player->username, PLAYER_NAME_MAX);

	return tcp_new_player;
}

static void 
broadcast_new_player(server_t* server, client_t* new_client)
{
	ght_t* clients = &server->clients;
	const net_tcp_new_player_t* new_player = client_to_tcp_new_player(server, new_client);

	GHT_FOREACH(client_t* client, clients, {
		if (client->player)
		{
			ssp_segbuf_add(&client->tcp_buf, NET_TCP_NEW_PLAYER, sizeof(net_tcp_new_player_t), new_player);
			if (client != new_client)
			{
				const net_tcp_new_player_t* other_player = client_to_tcp_new_player(server, client);
				ssp_segbuf_add(&new_client->tcp_buf, NET_TCP_NEW_PLAYER, sizeof(net_tcp_new_player_t), other_player);

				ssp_tcp_send_segbuf(&client->tcp_sock, &client->tcp_buf);
			}
		}
	});
	ssp_tcp_send_segbuf(&new_client->tcp_sock, &new_client->tcp_buf);
}

void 
broadcast_delete_player(server_t* server, u32 id)
{
	ght_t* clients = &server->clients;
	net_tcp_delete_player_t del_player = {id};

	GHT_FOREACH(client_t* client, clients, {
		ssp_segbuf_add(&client->tcp_buf, NET_TCP_DELETE_PLAYER, sizeof(net_tcp_delete_player_t), &del_player);
		ssp_tcp_send_segbuf(&client->tcp_sock, &client->tcp_buf);
	});
}

void
on_player_changed(cg_player_t* player, server_t* server)
{
	ght_t* clients = &server->clients;
	net_udp_player_move_t* move = mmframes_alloc(&server->mmf, sizeof(net_udp_player_move_t));
	move->player_id = player->id;
	move->pos = player->pos;
	move->input = player->input;
	move->absolute = false;

	GHT_FOREACH(client_t* client, clients, {
		ssp_segbuf_add(&client->udp_buf, NET_UDP_PLAYER_MOVE, sizeof(net_udp_player_move_t), move);
	});
}

void
server_on_player_gun_changed(cg_player_t* player, server_t* server)
{
	ght_t* clients = &server->clients;
	net_udp_player_gun_state_t* gun_state_out = mmframes_alloc(&server->mmf, sizeof(net_udp_player_gun_state_t));
	const cg_gun_t* gun = player->gun;

	gun_state_out->player_id = player->id;
	gun_state_out->gun_id = gun->spec->id;
	gun_state_out->ammo = gun->ammo;
	gun_state_out->bullet_timer = gun->bullet_timer;
	gun_state_out->charge_timer = gun->charge_time;
	gun_state_out->reload_timer = gun->reload_time;

	GHT_FOREACH(client_t* client, clients, 
	{
		ssp_segbuf_add_i(&client->udp_buf, NET_UDP_PLAYER_GUN_STATE, sizeof(net_udp_player_gun_state_t), gun_state_out);
	});
}

void
on_player_damaged(cg_player_t* target_player, cg_player_t* attacker_player, server_t* server)
{
	ght_t* clients = &server->clients;
	net_udp_player_health_t* health = mmframes_alloc(&server->mmf, sizeof(net_udp_player_health_t));
	net_udp_player_move_t* move = NULL;
	net_udp_player_stats_t* target_stats;
	net_udp_player_stats_t* attacker_stats;
	net_udp_player_died_t* player_died;
	health->player_id = target_player->id;
	health->health = target_player->health;

	if (target_player->health <= 0)
	{
		move = mmframes_alloc(&server->mmf, sizeof(net_udp_player_move_t));
		move->player_id = target_player->id;
		move->absolute = true;
		target_player->pos = move->pos = server_next_spawn(server);
		target_player->health = health->health = target_player->max_health;

		attacker_player->stats.kills++;
		target_player->stats.deaths++;

		player_died = mmframes_alloc(&server->mmf, sizeof(net_udp_player_died_t));
		player_died->target_player_id = target_player->id;
		player_died->attacker_player_id = attacker_player->id;

		target_stats = mmframes_alloc(&server->mmf, sizeof(net_udp_player_stats_t));
		target_stats->player_id = target_player->id;
		target_stats->kills = target_player->stats.kills;
		target_stats->deaths = target_player->stats.deaths;

		attacker_stats = mmframes_alloc(&server->mmf, sizeof(net_udp_player_stats_t));
		attacker_stats->player_id = attacker_player->id;
		attacker_stats->kills = attacker_player->stats.kills;
		attacker_stats->deaths = attacker_player->stats.deaths;

		printf("Player \"%s\" killed \"%s\".\n", attacker_player->username, target_player->username);
	}

	GHT_FOREACH(client_t* client, clients, {
		ssp_segbuf_add_i(&client->udp_buf, NET_UDP_PLAYER_HEALTH, sizeof(net_udp_player_health_t), health);
		if (move)
		{
			ssp_segbuf_add_i(&client->udp_buf, NET_UDP_PLAYER_MOVE, sizeof(net_udp_player_move_t), move);
			ssp_segbuf_add(&client->udp_buf, NET_UDP_PLAYER_DIED, sizeof(net_udp_player_died_t), player_died);
			ssp_segbuf_add(&client->udp_buf, NET_UDP_PLAYER_STATS, sizeof(net_udp_player_stats_t), target_stats);
			ssp_segbuf_add(&client->udp_buf, NET_UDP_PLAYER_STATS, sizeof(net_udp_player_stats_t), attacker_stats);
		}
	});
}

void 
client_tcp_connect(const ssp_segment_t* segment, server_t* server, client_t* client)
{
	net_tcp_sessionid_t* session = mmframes_alloc(&server->mmf, sizeof(net_tcp_sessionid_t));
	net_tcp_udp_info_t* udp_info = mmframes_alloc(&server->mmf, sizeof(net_tcp_udp_info_t));

	udp_info->port = server->udp_port;
	udp_info->tickrate = server->tickrate;
	udp_info->ssp_flags = SSP_FLAGS;
	udp_info->time = server->game.sbsm->present->timestamp;

	const net_tcp_connect_t* connect = (net_tcp_connect_t*)segment->data;

	client->player = coregame_add_player(&server->game, connect->username);
	client->player->user_data = client;
	coregame_create_gun(&server->game, CG_GUN_ID_SMALL, client->player);

	client->player->pos = server_next_spawn(server);

	session->session_id = client->session_id;
	session->player_id = client->player->id;

	printf("Client '%s' (%s) got %u for session ID.\n", 
			connect->username, client->tcp_sock.ipstr, session->session_id);

	ssp_segbuf_add(&client->tcp_buf, NET_TCP_SESSION_ID, sizeof(net_tcp_sessionid_t), session);
	ssp_segbuf_add(&client->tcp_buf, NET_TCP_CG_MAP, server->disk_map_size, server->disk_map);
	ssp_segbuf_add(&client->tcp_buf, NET_TCP_UDP_INFO, sizeof(net_tcp_udp_info_t), udp_info);

	const cg_gun_spec_t* gun_specs = (const cg_gun_spec_t*)server->game.gun_specs.buf;
	for (u32 i = 0; i < server->game.gun_specs.count; i++)
		ssp_segbuf_add(&client->tcp_buf, NET_TCP_GUN_SPEC, sizeof(cg_gun_spec_t), gun_specs + i);

	broadcast_new_player(server, client);
}

void 
player_cursor(const ssp_segment_t* segment, server_t* server, client_t* source_client)
{
	ght_t* clients = &server->clients;

	const net_udp_player_cursor_t* cursor = (const net_udp_player_cursor_t*)segment->data;
	source_client->player->cursor = cursor->cursor_pos;

	net_udp_player_cursor_t* new_cursor = mmframes_alloc(&server->mmf, sizeof(net_udp_player_cursor_t));
	new_cursor->cursor_pos = cursor->cursor_pos;
	new_cursor->player_id = source_client->player->id;

	GHT_FOREACH(client_t* client, clients, {
		if (client != source_client)
			ssp_segbuf_add(&client->udp_buf, NET_UDP_PLAYER_CURSOR, sizeof(net_udp_player_cursor_t), new_cursor);
	});
}

void 
udp_ping(const ssp_segment_t* segment, server_t* server, client_t* source_client)
{
	const net_udp_pingpong_t* in_ping = (const net_udp_pingpong_t*)segment->data;
	net_udp_pingpong_t out_ping = {
		.t_client_ms = in_ping->t_client_ms,
	};

	out_ping.t_server_ms = server->game.sbsm->present->timestamp;

	// insta send to client. No buffering. 
	ssp_packet_t* packet = ssp_insta_packet(&source_client->udp_buf, NET_UDP_PONG, &out_ping, sizeof(net_udp_pingpong_t));
	client_send(server, source_client, packet);
}

void 
player_ping(const ssp_segment_t* segment, server_t* server, client_t* source_client)
{
	const net_udp_player_ping_t* og_client_ping = (const net_udp_player_ping_t*)segment->data;
	if (og_client_ping->ms < 0)
	{
		printf("Player %u ping is less than zero: %f\n", og_client_ping->player_id, og_client_ping->ms);
		return;
	}

	net_udp_player_ping_t* client_ping = mmframes_alloc(&server->mmf, sizeof(net_udp_player_ping_t));
	ght_t* clients = &server->clients;

	ssp_segbuf_set_rtt(&source_client->udp_buf, og_client_ping->ms);

	client_ping->ms = og_client_ping->ms;
	client_ping->player_id = source_client->player->id;

	GHT_FOREACH(client_t* client, clients, {
		if (client != source_client)
			ssp_segbuf_add(&client->udp_buf, NET_UDP_PLAYER_PING, sizeof(net_udp_player_ping_t), client_ping);
	});
}

void 
want_server_stats(const ssp_segment_t* segment, UNUSED server_t* server, client_t* source_client)
{
	const net_tcp_want_server_stats_t* stats = (const net_tcp_want_server_stats_t*)segment->data;

	source_client->want_stats = stats->opt_in;
}

void 
chat_msg(const ssp_segment_t* segment, server_t* server, client_t* source_client)
{
	ght_t* clients = &server->clients;
	const net_tcp_chat_msg_t* src_msg = (const void*)segment->data;
	net_tcp_chat_msg_t new_msg = {
		.player_id = source_client->player->id
	};
	strncpy(new_msg.msg, src_msg->msg, CHAT_MSG_MAX);

	GHT_FOREACH(client_t* client, clients, {
		if (client->player)
		{
			ssp_segbuf_add(&client->tcp_buf, NET_TCP_CHAT_MSG, sizeof(net_tcp_chat_msg_t), &new_msg);
			ssp_tcp_send_segbuf(&client->tcp_sock, &client->tcp_buf);
		}
	});
}

void 
player_gun_id(const ssp_segment_t* segment, server_t* server, client_t* source_client)
{
	ght_t* clients = &server->clients;
	const net_udp_player_gun_id_t* player_gun_id = (const net_udp_player_gun_id_t*)segment->data;
	net_udp_player_gun_id_t* udp_player_gun_id;

	udp_player_gun_id = mmframes_alloc(&server->mmf, sizeof(net_udp_player_gun_id_t));
	udp_player_gun_id->player_id = source_client->player->id;

	if (coregame_player_change_gun(&server->game, source_client->player, player_gun_id->gun_id))
	{
		udp_player_gun_id->gun_id = player_gun_id->gun_id;

		GHT_FOREACH(client_t* client, clients, {
			if (client->player)
				ssp_segbuf_add_i(&client->udp_buf, NET_UDP_PLAYER_GUN_ID, sizeof(net_udp_player_gun_id_t), udp_player_gun_id);
		});
	}
	else
	{
		/* If changing gun failed, the the source client know */
		udp_player_gun_id->gun_id = source_client->player->gun->spec->id;

		ssp_segbuf_add_i(&source_client->udp_buf, NET_UDP_PLAYER_GUN_ID, sizeof(net_udp_player_gun_id_t), udp_player_gun_id);
	}
}

void 
player_input(const ssp_segment_t* segment, server_t* server, client_t* source_client)
{
	ght_t* clients = &server->clients;
	const net_udp_player_input_t* input_in = (void*)segment->data;
	net_udp_player_input_t* input_out = mmframes_alloc(&server->mmf, sizeof(net_udp_player_input_t));

	coregame_set_player_input_t(&server->game, source_client->player, input_in->flags, input_in->timestamp);

	input_out->player_id = source_client->player->id;
	input_out->flags = input_in->flags;
	input_out->timestamp = input_in->timestamp;

	GHT_FOREACH(client_t* client, clients, 
	{
		if (client->player && client != source_client)
			ssp_segbuf_add_i(&client->udp_buf, NET_UDP_PLAYER_INPUT, sizeof(net_udp_player_input_t), input_out);
	});
}

void 
player_reload(UNUSED const ssp_segment_t* segment, server_t* server, client_t* source_client)
{
	source_client->player->gun->ammo = 0;

	server_on_player_reload(source_client->player, server);
}

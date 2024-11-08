#include "server_game.h"

static void 
broadcast_new_player(server_t* server, client_t* new_client)
{
	ght_t* clients = &server->clients;

	GHT_FOREACH(client_t* client, clients, {
		if (client->player)
		{
			ssp_segbuff_add(&client->tcp_buf, NET_TCP_NEW_PLAYER, sizeof(cg_player_t), new_client->player);
			if (client != new_client)
			{
				ssp_segbuff_add(&new_client->tcp_buf, NET_TCP_NEW_PLAYER, sizeof(cg_player_t), client->player);
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
		ssp_segbuff_add(&client->tcp_buf, NET_TCP_DELETE_PLAYER, sizeof(net_tcp_delete_player_t), &del_player);
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
	move->dir = player->dir;

	GHT_FOREACH(client_t* client, clients, {
		ssp_segbuff_add(&client->udp_buf, NET_UDP_PLAYER_MOVE, sizeof(net_udp_player_move_t), move);
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
		target_player->pos = move->pos = server_next_spawn(server);
		target_player->dir = move->dir = vec2f(0, 0);
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
		ssp_segbuff_add(&client->udp_buf, NET_UDP_PLAYER_HEALTH, sizeof(net_udp_player_health_t), health);
		if (move)
		{
			ssp_segbuff_add(&client->udp_buf, NET_UDP_PLAYER_MOVE, sizeof(net_udp_player_move_t), move);
			ssp_segbuff_add(&client->udp_buf, NET_UDP_PLAYER_DIED, sizeof(net_udp_player_died_t), player_died);
			ssp_segbuff_add(&client->udp_buf, NET_UDP_PLAYER_STATS, sizeof(net_udp_player_stats_t), target_stats);
			ssp_segbuff_add(&client->udp_buf, NET_UDP_PLAYER_STATS, sizeof(net_udp_player_stats_t), attacker_stats);
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

	const net_tcp_connect_t* connect = (net_tcp_connect_t*)segment->data;

	client->player = coregame_add_player(&server->game, connect->username);
	client->player->pos = server_next_spawn(server);

	session->session_id = client->session_id;
	session->player_id = client->player->id;

	printf("Client '%s' (%s) got %u for session ID.\n", 
			connect->username, client->tcp_sock.ipstr, session->session_id);

	ssp_segbuff_add(&client->tcp_buf, NET_TCP_SESSION_ID, sizeof(net_tcp_sessionid_t), session);
	ssp_segbuff_add(&client->tcp_buf, NET_TCP_CG_MAP, server->disk_map_size, server->disk_map);
	ssp_segbuff_add(&client->tcp_buf, NET_TCP_UDP_INFO, sizeof(net_tcp_udp_info_t), udp_info);
	broadcast_new_player(server, client);
}

void 
player_dir(const ssp_segment_t* segment, UNUSED server_t* server, client_t* source_client)
{
	const net_udp_player_dir_t* dir = (const net_udp_player_dir_t*)segment->data;

	source_client->player->dir = dir->dir;
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
			ssp_segbuff_add(&client->udp_buf, NET_UDP_PLAYER_CURSOR, sizeof(net_udp_player_cursor_t), new_cursor);
	});
}

void 
player_shoot(const ssp_segment_t* segment, server_t* server, client_t* source_client)
{
	ght_t* clients = &server->clients;

	const net_udp_player_shoot_t* shoot = (const net_udp_player_shoot_t*)segment->data;

	coregame_player_shoot(&server->game, source_client->player, shoot->shoot_dir);

	net_udp_player_shoot_t* new_shoot = mmframes_alloc(&server->mmf, sizeof(net_udp_player_shoot_t));
	new_shoot->player_id = source_client->player->id;
	new_shoot->shoot_dir = shoot->shoot_dir;

	GHT_FOREACH(client_t* client, clients, {
		if (client != source_client)
			ssp_segbuff_add(&client->udp_buf, NET_UDP_PLAYER_SHOOT, sizeof(net_udp_player_shoot_t), new_shoot);
	});
}

void 
udp_ping(const ssp_segment_t* segment, server_t* server, client_t* source_client)
{
	const net_udp_pingpong_t* ping = (const net_udp_pingpong_t*)segment->data;

	// insta send to client. No buffering. 
	ssp_packet_t* packet = ssp_insta_packet(&source_client->udp_buf, NET_UDP_PONG, ping, sizeof(net_udp_pingpong_t));
	client_send(server, source_client, packet);
}

void 
player_ping(const ssp_segment_t* segment, server_t* server, client_t* source_client)
{
	const net_udp_player_ping_t* og_client_ping = (const net_udp_player_ping_t*)segment->data;
	net_udp_player_ping_t* client_ping = mmframes_alloc(&server->mmf, sizeof(net_udp_player_ping_t));
	ght_t* clients = &server->clients;

	client_ping->ms = og_client_ping->ms;
	client_ping->player_id = source_client->player->id;

	GHT_FOREACH(client_t* client, clients, {
		if (client != source_client)
			ssp_segbuff_add(&client->udp_buf, NET_UDP_PLAYER_PING, sizeof(net_udp_player_ping_t), client_ping);
	});
}

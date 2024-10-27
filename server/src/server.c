#define _GNU_SOURCE
#include "server.h"
#include <sys/random.h>
#include <time.h>
#include <string.h>

#define PORT 8080

static i32
server_init_tcp(server_t* server)
{
	i32 ret;

	ret = ssp_tcp_server(&server->tcp_sock, SSP_IPv4, server->port);

	return ret;
}

static i32
server_init_udp(server_t* server)
{
	if ((server->udp_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		perror("socket UDP");
		return -1;
	}

	server->tcp_sock.addr.sockaddr.in.sin_port = htons(server->udp_port);
	if (bind(server->udp_fd, (struct sockaddr*)&server->tcp_sock.addr.sockaddr, server->tcp_sock.addr.addr_len) == -1)
	{
		perror("bind UDP");
		return -1;
	}

	return 0;
}

static void
read_client(server_t* server, event_t* event)
{
	client_t* client = event->data;
	char buffer[1024] = {0};

	ssize_t bytes_read;

	if ((bytes_read = recv(event->fd, buffer, 1024, 0)) == -1)
	{
		perror("recv");
		server_close_event(server, event);
	}
	else if (bytes_read == 0)
	{
		server_close_event(server, event);
	}
	else
	{
		ssp_parse_buf(&server->netdef.ssp_state, buffer, bytes_read, client);
	}
}

static void 
broadcast_delete_player(server_t* server, u32 id)
{
	ght_t* clients = &server->clients;
	net_tcp_delete_player_t del_player = {id};

	GHT_FOREACH(client_t* client, clients, {
		ssp_segbuff_add(&client->tcp_buf, NET_TCP_DELETE_PLAYER, sizeof(net_tcp_delete_player_t), &del_player);
		ssp_tcp_send_segbuf(&client->tcp_sock, &client->tcp_buf);
	});
}

static void
close_client(server_t* server, event_t* event)
{
	u32 player_id = 0;
	client_t* client = event->data;

	ssp_tcp_sock_close(&client->tcp_sock);
	printf("Client '%s' closed.\n", client->tcp_sock.ipstr);

	if (client->player)
	{
		player_id = client->player->id;
		coregame_free_player(&server->game, client->player);
	}

	ght_del(&server->clients, client->session_id);

	if (player_id)
		broadcast_delete_player(server, player_id);
}

static void
handle_new_connection(server_t* server, UNUSED event_t* event)
{
	client_t* client = accept_client(server);
	if (client == NULL)
		return;

	server_add_event(server, client->tcp_sock.sockfd, client, read_client, close_client);
}

static void 
read_udp_packet(server_t* server, event_t* event)
{
	void* buf = malloc(1024);
	i64 bytes_read;
	udp_addr_t info = {
		.addr_len = sizeof(struct sockaddr_in)
	};

	if ((bytes_read = recvfrom(event->fd, buf, 1024, 0, &info.addr, &info.addr_len)) == -1)
	{
		perror("recvfrom");
		return;
	}

	ssp_parse_buf(&server->netdef.ssp_state, buf, bytes_read, &info);
	free(buf);
}

static i32
server_init_epoll(server_t* server)
{
	if ((server->epfd = epoll_create1(EPOLL_CLOEXEC)) == -1)
	{
		perror("epoll_create1");
		return -1;
	}

	server_add_event(server, server->tcp_sock.sockfd, NULL, handle_new_connection, NULL);
	server_add_event(server, server->udp_fd, NULL, read_udp_packet, NULL);

	return 0;
}

static void
server_init_coregame(server_t* server)
{
	coregame_init(&server->game);
}

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

static void 
client_tcp_connect(const ssp_segment_t* segment, server_t* server, client_t* client)
{
	net_tcp_sessionid_t sessionid;
	net_tcp_udp_info_t udp_info = {server->udp_port};
	const net_tcp_connect_t* connect = (net_tcp_connect_t*)segment->data;

	client->player = coregame_add_player(&server->game, connect->username);
	sessionid.session_id = client->session_id;
	sessionid.player_id = client->player->id;

	printf("Client '%s' (%s) got %u for session ID.\n", connect->username, client->tcp_sock.ipstr, sessionid.session_id);

	ssp_segbuff_add(&client->tcp_buf, NET_TCP_SESSION_ID, sizeof(net_tcp_sessionid_t), &sessionid);
	ssp_segbuff_add(&client->tcp_buf, NET_TCP_UDP_INFO, sizeof(net_tcp_udp_info_t), &udp_info);
	broadcast_new_player(server, client);
}

static bool 
verify_session(u32 session_id, server_t* server, udp_addr_t* source_data, void** new_source)
{
	client_t* client;

	client = ght_get(&server->clients, session_id);
	if (client == NULL)
	{
		printf("No client with session ID: %u\n", session_id);
		return false;
	}

	if (client->tcp_sock.addr.sockaddr.in.sin_addr.s_addr != source_data->addr.sin_addr.s_addr)
	{
		printf("Client TCP address != source_data\n");
		return false;
	}

	*new_source = client;
	memcpy(&client->udp, source_data, sizeof(udp_addr_t));
	client->udp_connected = true;

	return true;
}

static void 
player_move(const ssp_segment_t* segment, server_t* server, client_t* source_client)
{
	ght_t* clients = &server->clients;

	const net_udp_player_move_t* move = (const net_udp_player_move_t*)segment->data;

	source_client->player->dir = move->dir;
	source_client->player->pos = move->pos;

	net_udp_player_move_t* new_move = mmframes_alloc(&server->mmf, sizeof(net_udp_player_move_t));
	new_move->player_id = source_client->player->id;
	new_move->dir = move->dir;
	new_move->pos = move->pos;

	GHT_FOREACH(client_t* client, clients, {
		ssp_segbuff_add(&client->udp_buf, NET_UDP_PLAYER_MOVE, 
				  sizeof(net_udp_player_move_t), 
				  new_move);
	});
}

static void 
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

static void 
player_shoot(const ssp_segment_t* segment, server_t* server, client_t* source_client)
{
	ght_t* clients = &server->clients;

	const net_udp_player_shoot_t* shoot = (const net_udp_player_shoot_t*)segment->data;

	coregame_player_shoot(&server->game, source_client->player, shoot->shoot_dir);

	net_udp_player_shoot_t* new_shoot = mmframes_alloc(&server->mmf, sizeof(net_udp_player_shoot_t));
	new_shoot->player_id = source_client->player->id;
	new_shoot->shoot_dir = shoot->shoot_dir;
	new_shoot->shoot_pos = shoot->shoot_pos;

	GHT_FOREACH(client_t* client, clients, {
		if (client != source_client)
			ssp_segbuff_add(&client->udp_buf, NET_UDP_PLAYER_SHOOT, sizeof(net_udp_player_shoot_t), new_shoot);
	});
}

static void
server_init_netdef(server_t* server)
{
	ssp_segmap_callback_t callbacks[NET_SEGTYPES_LEN] = {0};
	callbacks[NET_TCP_CONNECT] = (ssp_segmap_callback_t)client_tcp_connect;
	callbacks[NET_UDP_PLAYER_MOVE] = (ssp_segmap_callback_t)player_move;
	callbacks[NET_UDP_PLAYER_CURSOR] = (ssp_segmap_callback_t)player_cursor;
	callbacks[NET_UDP_PLAYER_SHOOT] = (ssp_segmap_callback_t)player_shoot;

	netdef_init(&server->netdef, NULL, callbacks);
	server->netdef.ssp_state.user_data = server;
	server->netdef.ssp_state.verify_session = (ssp_session_verify_callback_t)verify_session;
}

i32 
server_init(server_t* server, UNUSED i32 argc, UNUSED const char** argv)
{
	printf("server_init()...\n");

	server->port = PORT;
	server->udp_port = PORT + 1;

	ght_init(&server->clients, 10, free);

	if (server_init_tcp(server) == -1)
		goto err;
	if (server_init_udp(server) == -1)
		goto err;
	if (server_init_epoll(server) == -1)
		goto err;
	server_init_netdef(server);
	server_init_coregame(server);
	mmframes_init(&server->mmf);

	server->running = true;

	return 0;
err:
	server_cleanup(server);
	return -1;
}

static void 
server_poll(server_t* server)
{
	i32 nfds;
	i32 timeout;

	do {
		timeout = (server->clients.count == 0) ? -1 : 0;

		if ((nfds = epoll_wait(server->epfd, server->events, MAX_EVENTS, timeout)) == -1)
		{
			perror("epoll_wait");
			server->running = false;
		}

		for (i32 i = 0; i < nfds; i++)
		{
			struct epoll_event* event = server->events + i;
			server_handle_event(server, event->data.ptr, event->events);
		}
	} while (nfds);
}

static void 
server_flush_udp_clients(server_t* server)
{
	ght_t* clients = &server->clients;

	GHT_FOREACH(client_t* client, clients, {
		if (client->udp_connected)
		{
			ssp_packet_t* packet = ssp_serialize_packet(&client->udp_buf);
			if (packet)
			{
				u32 packet_size = ssp_packet_size(packet);
				if (sendto(server->udp_fd, packet, packet_size, 0, &client->udp.addr, client->udp.addr_len) == -1)
				{
					perror("sendto");
				}
				free(packet);
			}
		}
	});
}

void 
server_run(server_t* server)
{
	printf("server_run()...\n");

	f64 fps = 64.0;

	struct timespec start_time, end_time, sleep_duration, prev_time;
	f64 target_frame_time = 1.0 / fps;

	while (server->running)
	{
		clock_gettime(CLOCK_MONOTONIC, &start_time);

		server_poll(server);
		coregame_update(&server->game);
		server_flush_udp_clients(server);
		mmframes_clear(&server->mmf);

		clock_gettime(CLOCK_MONOTONIC, &end_time);
		f64 elapsed_time =	(end_time.tv_sec - start_time.tv_sec) + 
							(end_time.tv_nsec - start_time.tv_nsec) / 1e9;
		// f64 frame_time =	(start_time.tv_sec - prev_time.tv_sec) + 
		// 					(start_time.tv_nsec - prev_time.tv_nsec) / 1e9;
		// f64 time_ms = frame_time * 1000.0;
		// printf("Frame time: %f\n", time_ms);

		memcpy(&prev_time, &start_time, sizeof(struct timespec));

		f64 sleep_time = target_frame_time - elapsed_time;
		if (sleep_time > 0)
		{
			sleep_duration.tv_sec = (time_t)sleep_time;
			sleep_duration.tv_nsec = (sleep_time - sleep_duration.tv_sec) * 1e9;
			nanosleep(&sleep_duration, NULL);
		}
	}
}

void 
server_cleanup(UNUSED server_t* server)
{
	printf("server_cleanup()...\n");
}

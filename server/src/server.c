#define _GNU_SOURCE
#include "server.h"
#include <sys/random.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <sys/signalfd.h>
#include <signal.h>

#define RECV_BUFFER_SIZE 4096
#define PORT 8080
#define TICKRATE 64.0

static i32
server_init_tcp(server_t* server)
{
	i32 ret;

	ret = ssp_tcp_server(&server->tcp_sock, SSP_IPv4, server->port);

	return ret;
}

static vec2f_t 
server_next_spawn(server_t* server)
{
	vec2f_t ret = server->spawn_points[server->spawn_idx];

	server->spawn_idx++;
	if (server->spawn_idx >= MAX_SPAWN_POINTS)
		server->spawn_idx = 0;
	return ret;
}

static void
server_set_tickrate(server_t* server, f64 tickrate)
{
	server->tickrate = tickrate;
	server->interval = 1.0 / tickrate;
	server->interval_ns = (1.0 / tickrate) * 1e9;
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
	void* buf = malloc(RECV_BUFFER_SIZE);
	i32 ret;

	i64 bytes_read;

	if ((bytes_read = recv(event->fd, buf, RECV_BUFFER_SIZE, 0)) == -1)
	{
		perror("recv");
		server_close_event(server, event);
	}
	else if (bytes_read == 0)
		server_close_event(server, event);
	else
	{
		ret = ssp_parse_buf(&server->netdef.ssp_state, &client->udp_buf, buf, bytes_read, client);
		if (ret == SSP_FAILED)
		{
			printf("Client (%s) sent invalid packet. Closing client.\n",
				client->tcp_sock.ipstr);
			server_close_event(server, event);
		}
	}

	free(buf);
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
close_client(server_t* server, client_t* client)
{
	u32 player_id = 0;

	ssp_tcp_sock_close(&client->tcp_sock);
	printf("Client (%s) closed.\n", client->tcp_sock.ipstr);

	if (client->player)
	{
		player_id = client->player->id;
		coregame_free_player(&server->game, client->player);
	}

	ssp_segbuff_destroy(&client->udp_buf);
	ssp_segbuff_destroy(&client->tcp_buf);
	ght_del(&server->clients, client->session_id);

	if (player_id)
		broadcast_delete_player(server, player_id);
}

static void
event_close_client(server_t* server, event_t* event)
{
	close_client(server, event->data);
}

static void
handle_new_connection(server_t* server, UNUSED event_t* event)
{
	client_t* client = accept_client(server);
	if (client == NULL)
		return;

	server_add_event(server, client->tcp_sock.sockfd, client, read_client, event_close_client);
}

static void
set_udp_info(udp_addr_t* info)
{
	inet_ntop(AF_INET, &info->addr.sin_addr, info->ipaddr, INET6_ADDRSTRLEN);
	info->port = ntohs(info->addr.sin_port);
}

static void 
read_udp_packet(server_t* server, event_t* event)
{
	void* buf = malloc(RECV_BUFFER_SIZE);
	i64 bytes_read;
	i32 ret;
	udp_addr_t info = {
		.addr_len = sizeof(struct sockaddr_in)
	};

	if ((bytes_read = recvfrom(event->fd, buf, RECV_BUFFER_SIZE, 0, &info.addr, &info.addr_len)) == -1)
	{
		perror("recvfrom");
		free(buf);
		return;
	}

	set_udp_info(&info);

	ret = ssp_parse_buf(&server->netdef.ssp_state, NULL, buf, bytes_read, &info);
	if (ret == SSP_FAILED)
	{
		printf("Invalid UDP packet (%zu bytes) from %s:%u.\n", bytes_read, info.ipaddr, info.port);
	}
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

static void
on_player_damaged(cg_player_t* player, server_t* server)
{
	ght_t* clients = &server->clients;
	net_udp_player_health_t* health = mmframes_alloc(&server->mmf, sizeof(net_udp_player_health_t));
	net_udp_player_move_t* move = NULL;
	health->player_id = player->id;
	health->health = player->health;

	if (player->health <= 0)
	{
		move = mmframes_alloc(&server->mmf, sizeof(net_udp_player_move_t));
		move->player_id = player->id;
		player->pos = move->pos = server_next_spawn(server);
		player->dir = move->dir = vec2f(0, 0);
		player->health = health->health = player->max_health;
	}

	GHT_FOREACH(client_t* client, clients, {
		ssp_segbuff_add(&client->udp_buf, NET_UDP_PLAYER_HEALTH, sizeof(net_udp_player_health_t), health);
		if (move)
			ssp_segbuff_add(&client->udp_buf, NET_UDP_PLAYER_MOVE, sizeof(net_udp_player_move_t), move);
	});
}

static void
server_init_coregame(server_t* server)
{
	coregame_init(&server->game, false);
	server->game.user_data = server;
	server->game.player_changed = (cg_player_changed_callback_t)on_player_changed;
	server->game.player_damaged = (cg_player_changed_callback_t)on_player_damaged;

	f32 offset = 100.0;
	const cg_rect_t* wb = &server->game.world_border;

	// Top Left
	server->spawn_points[0] = vec2f(
		wb->pos.x + offset,
		wb->pos.y + offset
	);

	// Top Middle
	server->spawn_points[1] = vec2f(
		wb->pos.x + (wb->size.x / 2),
		wb->pos.y + offset
	);

	// Top Right 
	server->spawn_points[2] = vec2f(
		wb->pos.x + wb->size.x - (offset * 2),
		wb->pos.y + offset
	);
	
	// Middle Right 
	server->spawn_points[3] = vec2f(
		wb->pos.x + wb->size.x - (offset * 2),
		wb->pos.y + (wb->size.y / 2)
	);
	
	// Bottom Right 
	server->spawn_points[4] = vec2f(
		wb->pos.x + wb->size.x - (offset * 2),
		wb->pos.y + wb->size.y - (offset * 2)
	);
	
	// Bottom Middle 
	server->spawn_points[5] = vec2f(
		wb->pos.x + (wb->size.x / 2),
		wb->pos.y + wb->size.y - (offset * 2)
	);

	// Bottom Left 
	server->spawn_points[6] = vec2f(
		wb->pos.x + offset,
		wb->pos.y + wb->size.y - (offset * 2)
	);
	
	// Middle Left 
	server->spawn_points[7] = vec2f(
		wb->pos.x + offset,
		wb->pos.y + (wb->size.y / 2)
	);
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
	ssp_segbuff_add(&client->tcp_buf, NET_TCP_UDP_INFO, sizeof(net_tcp_udp_info_t), &udp_info);
	broadcast_new_player(server, client);
}

static bool 
verify_session(u32 session_id, server_t* server, udp_addr_t* source_data, void** new_source, ssp_segbuff_t** segbuf)
{
	client_t* client;

	client = ght_get(&server->clients, session_id);
	if (client == NULL)
	{
		printf("No client with session ID: %u\n", session_id);
		return false;
	}

	if (client->udp_connected)
	{
		if (client->udp.addr.sin_addr.s_addr != source_data->addr.sin_addr.s_addr)
		{
			printf("UDP \"connected\" client IP Address dones't match from new packet. (og: %s != new: %s)...\n",
					client->udp.ipaddr, source_data->ipaddr);
		}

		if (client->udp.addr.sin_port != source_data->addr.sin_port)
		{
			printf("UDP \"connected\" client source port dones't match from new packet. (og: %u != new: %u)...\n",
					client->udp.port, source_data->port);
		}
	}
	else if (client->tcp_sock.addr.sockaddr.in.sin_addr.s_addr != source_data->addr.sin_addr.s_addr)
	{
		printf("Client TCP IP Address (%s) != to UDP Address (%s)!\n",
				client->tcp_sock.ipstr, source_data->ipaddr);
		return false;
	}
	else
	{
		memcpy(&client->udp, source_data, sizeof(udp_addr_t));
		client->udp_connected = true;
	}
	*new_source = client;
	*segbuf = &client->udp_buf;

	return true;
}

static void 
player_dir(const ssp_segment_t* segment, UNUSED server_t* server, client_t* source_client)
{
	const net_udp_player_dir_t* dir = (const net_udp_player_dir_t*)segment->data;

	source_client->player->dir = dir->dir;
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
udp_ping(const ssp_segment_t* segment, server_t* server, client_t* source_client)
{
	const net_udp_pingpong_t* ping = (const net_udp_pingpong_t*)segment->data;

	// insta send to client. No buffering. 
	ssp_packet_t* packet = ssp_insta_packet(&source_client->udp_buf, NET_UDP_PONG, ping, sizeof(net_udp_pingpong_t));
	client_send(server, source_client, packet);
}

static void
server_init_netdef(server_t* server)
{
	ssp_segmap_callback_t callbacks[NET_SEGTYPES_LEN] = {0};
	callbacks[NET_TCP_CONNECT] = (ssp_segmap_callback_t)client_tcp_connect;
	callbacks[NET_UDP_PLAYER_DIR] = (ssp_segmap_callback_t)player_dir;
	callbacks[NET_UDP_PLAYER_CURSOR] = (ssp_segmap_callback_t)player_cursor;
	callbacks[NET_UDP_PLAYER_SHOOT] = (ssp_segmap_callback_t)player_shoot;
	callbacks[NET_UDP_PING] = (ssp_segmap_callback_t)udp_ping;

	netdef_init(&server->netdef, NULL, callbacks);
	server->netdef.ssp_state.user_data = server;
	server->netdef.ssp_state.verify_session = (ssp_session_verify_callback_t)verify_session;
}

static void
signalfd_read(server_t* server, event_t* event)
{
	struct signalfd_siginfo siginfo;
	const i64 size = sizeof(struct signalfd_siginfo);
	i32 sig;

	if (read(event->fd, &siginfo, size) != size)
	{
		perror("signalfd_read");
		server->running = false;
		return;
	}
	sig = siginfo.ssi_signo;

	printf("\nSignal recieved: %d (%s)\n", sig, strsignal(sig));

	switch (sig)
	{
		case SIGINT:
		case SIGTERM:
			server->running = false;
			break;
		default:
			break;
	}
}

static void
signalfd_close(server_t* server)
{
	if (server->signalfd >= 0)
		return;

	if (close(server->signalfd) == -1)
		perror("signalfd_close");
}

static void
event_signalfd_close(server_t* server, UNUSED event_t* ev)
{
	signalfd_close(server);
}

static i32
server_init_signalfd(server_t* server)
{
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGTERM);
	sigprocmask(SIG_BLOCK, &mask, NULL);

	server->signalfd = signalfd(-1, &mask, 0);
	if (server->signalfd == -1)
	{
		perror("signalfd");
		return -1;
	}

	server_add_event(server, server->signalfd, NULL, signalfd_read, event_signalfd_close);

	return 0;
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
	if (server_init_signalfd(server) == -1)
		goto err;
	server_init_netdef(server);
	server_init_coregame(server);
	mmframes_init(&server->mmf);

	server_set_tickrate(server, TICKRATE);

	server->running = true;

	return 0;
err:
	server_cleanup(server);
	return -1;
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
				client_send(server, client, packet);
		}
	});
}

static inline void
ns_to_timespec(struct timespec* timespec, i64 ns)
{
	timespec->tv_sec = ns / 1e9;
	timespec->tv_nsec = ns % (i64)1e9;
}

static void 
format_ns(char* buf, u64 max, i64 ns)
{
	if (ns < 1e6)
		snprintf(buf, max, "%ld ns", ns);
	else
		snprintf(buf, max, "%.3f ms", (f64)ns / 1e6);
}

static void
print_frametimes(i64 ns, const char* high_str)
{
	char frametime_str[FRAMETIMES_LEN];
	format_ns(frametime_str, FRAMETIMES_LEN, ns);

	// Go one line up, clear it and print.
	printf("\033[F\033[KFrame time: %s\t(highest: %s)\n", frametime_str, high_str);
}

static void
server_poll(server_t* server, struct timespec* prev_start_time, struct timespec* prev_end_time)
{
	i32 nfds;
	i64 prev_frame_time_ns;
	i64 elapsed_time_ns;
	i64 timeout_time_ns = 0;
	u32 errors = 0;
	struct timespec timeout = {0};
	struct timespec current_time;
	struct timespec* do_timeout = (server->clients.count == 0) ? NULL : &timeout;
	struct epoll_event* event;

	if (do_timeout)
	{
		prev_frame_time_ns = ((prev_end_time->tv_sec - prev_start_time->tv_sec) * 1e9) + 
							 (prev_end_time->tv_nsec - prev_start_time->tv_nsec);
		if (prev_frame_time_ns > server->highest_frametime)
		{
			server->highest_frametime = prev_frame_time_ns;
			format_ns(server->highest_frametime_str, FRAMETIME_LEN, prev_frame_time_ns);
		}
		print_frametimes(prev_frame_time_ns, server->highest_frametime_str);
		timeout_time_ns = server->interval_ns - prev_frame_time_ns;
		if (timeout_time_ns > 0)
			ns_to_timespec(&timeout, timeout_time_ns);
	}

	do {
		do_timeout = (server->clients.count == 0) ? NULL : &timeout;

		nfds = epoll_pwait2(server->epfd, server->ep_events, MAX_EVENTS, do_timeout, NULL);
		if (nfds == -1)
		{
			if (errno == EINTR)
				continue;
			perror("epoll_pwait2");
			errors++;
			if (errors >= 3)
			{
				server->running = false;
				break;
			}
		}

		for (i32 i = 0; i < nfds; i++)
		{
			event = server->ep_events + i;
			server_handle_event(server, event->data.ptr, event->events);
		}

		if (do_timeout)
		{
			clock_gettime(CLOCK_MONOTONIC, &current_time);
			elapsed_time_ns =	((current_time.tv_sec - prev_end_time->tv_sec) * 1e9) +
								(current_time.tv_nsec - prev_end_time->tv_nsec);
			memcpy(prev_end_time, &current_time, sizeof(struct timespec));
			timeout_time_ns -= elapsed_time_ns;
			if (timeout_time_ns > 0)
				ns_to_timespec(&timeout, timeout_time_ns);
			else
				timeout.tv_sec = timeout.tv_nsec = 0;
		}
	} while ((nfds || timeout_time_ns > 0) && server->running);
}

void 
server_run(server_t* server)
{
	printf("server_run()...\n");

	struct timespec start_time, end_time, prev_time;

	clock_gettime(CLOCK_MONOTONIC, &start_time);
	memcpy(&end_time, &start_time, sizeof(struct timespec));
	memcpy(&prev_time, &start_time, sizeof(struct timespec));

	while (server->running)
	{
		server_poll(server, &start_time, &end_time);

		clock_gettime(CLOCK_MONOTONIC, &start_time);

		coregame_update(&server->game);
		server_flush_udp_clients(server);
		mmframes_clear(&server->mmf);

		clock_gettime(CLOCK_MONOTONIC, &end_time);
	}
}

static void
server_cleanup_clients(server_t* server)
{
	ght_t* clients = &server->clients;

	GHT_FOREACH(client_t* client, clients, {
		if (client->player)
		{
			ssp_segbuff_add(&client->tcp_buf, NET_TCP_SERVER_SHUTDOWN, 0, NULL);
			ssp_tcp_send_segbuf(&client->tcp_sock, &client->tcp_buf);
		}
		close_client(server, client);
	});

	ght_destroy(&server->clients);
}

void 
server_cleanup(server_t* server)
{
	server_cleanup_clients(server);

	server_close_all_events(server);

	ssp_tcp_sock_close(&server->tcp_sock);
	signalfd_close(server);

	if (server->udp_fd > 0 && close(server->udp_fd) == -1)
		perror("close udp fd");

	if (server->epfd > 0 && close(server->epfd) == -1)
		perror("close epoll");

	coregame_cleanup(&server->game);
	mmframes_free(&server->mmf);
	netdef_destroy(&server->netdef);
}

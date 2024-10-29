#define _GNU_SOURCE
#include "client_net.h"
#include "app.h"
#include "player.h"
#include <time.h>

#ifdef __linux__
#include <sys/epoll.h>
#include "app_wayland.h"
#include <errno.h>
#endif

#define BUFFER_SIZE 4096
#define MAX_EVENTS 4

static f64 
get_elapsed_time(const struct timespec* current_time, const struct timespec* start_time)
{
	f64 elapsed_time =	(current_time->tv_sec - start_time->tv_sec) +
						(current_time->tv_nsec - start_time->tv_nsec) / 1e9;
	return elapsed_time;
}

static void 
session_id(const ssp_segment_t* segment, waapp_t* app, UNUSED void* source_data)
{
	const net_tcp_sessionid_t* sessionid = (const net_tcp_sessionid_t*)segment->data;

	app->net.session_id = sessionid->session_id;
	app->net.player_id = sessionid->player_id;
	app->net.udp.buf.session_id = sessionid->session_id;

	printf("Got Session ID: %u, player ID: %u\n",
		sessionid->session_id, sessionid->player_id);
}

void 
client_net_add_fdevent(waapp_t* app, sock_t fd, fdevent_callback_t read, fdevent_callback_t close, void* data)
{
	client_net_t* net = &app->net;
	fdevent_t* event = array_add_into(&net->events);
	event->fd = fd;
	event->read = read;
	event->close = close;
	event->data = data;

#ifdef __linux__
	struct epoll_event ev = {
		.data.ptr = event,
		.events = EPOLLIN | EPOLLHUP
	};

	if (epoll_ctl(net->epfd, EPOLL_CTL_ADD, fd, &ev) == -1)
		perror("epoll_ctl ADD");
#endif
}

static void
tcp_close(UNUSED waapp_t* app, fdevent_t* fdev)
{
	ssp_tcp_sock_t* sock = fdev->data;

#ifdef __linux__
	if (epoll_ctl(app->net.epfd, EPOLL_CTL_DEL, fdev->fd, NULL) == -1)
		perror("epoll_ctl DEL");
#endif

	ssp_tcp_sock_close(sock);
	// TODO: Remove fdevent_t from app->net.events
	// free(fdev);

	printf("TCP Socket Disconnected.\n");

}

static void
tcp_read(waapp_t* app, fdevent_t* fdev)
{
	void* buf = malloc(BUFFER_SIZE);
	i64 bytes_read;

	if ((bytes_read = recv(fdev->fd, buf, BUFFER_SIZE, 0)) == -1)
	{
		perror("recv");
		fdev->close(app, fdev);
	}
	else if (bytes_read == 0)
	{
		fdev->close(app, fdev);
	}
	else
	{
		ssp_parse_buf(&app->net.def.ssp_state, NULL, buf, bytes_read, NULL);
	}
	free(buf);
}

static void 
new_player(const ssp_segment_t* segment, waapp_t* app, UNUSED void* user_data)
{
	const cg_player_t* new_player = (const cg_player_t*)segment->data;

	cg_player_t* cg_player = malloc(sizeof(cg_player_t));
	memcpy(cg_player, new_player, sizeof(cg_player_t));

	player_t* player = player_new_from(app, cg_player);

	if (cg_player->id == app->net.player_id)
	{
		app->player = player;
		player->hpbar.fill.color = rgba(0x00FF00FF);
	}

	coregame_add_player_from(&app->game, cg_player);
}

static void 
delete_player(const ssp_segment_t* segment, waapp_t* app, UNUSED void* user_data)
{
	const net_tcp_delete_player_t* del_player = (const net_tcp_delete_player_t*)segment->data;

	cg_player_t* player = ght_get(&app->game.players, del_player->player_id);
	if (player)
	{
		coregame_free_player(&app->game, player);
	}
}

static void
udp_info(const ssp_segment_t* segment, waapp_t* app, UNUSED void* source_data)
{
	net_tcp_udp_info_t* info = (net_tcp_udp_info_t*)segment->data;
	printf("UDP Server Port: %u\n", info->port);
	app->net.udp.server.addr.sin_port = htons(info->port);
	client_net_set_tickrate(app, info->tickrate);

	app->net.udp.port = info->port;
}

static void
udp_read(waapp_t* app, fdevent_t* fdev)
{
	client_net_t* net = &app->net;

	void* buf = malloc(BUFFER_SIZE);
	i64 bytes_read;
	udp_addr_t addr = {
		.addr_len = sizeof(struct sockaddr_in)
	};

	if ((bytes_read = recvfrom(fdev->fd, buf, BUFFER_SIZE, 0, (struct sockaddr*)&addr.addr, &addr.addr_len)) == -1)
	{
		perror("recvfrom");
		return;
	}

	net->udp.in.bytes += bytes_read;
	net->udp.in.count++;

	ssp_parse_buf(&net->def.ssp_state, &net->udp.buf, buf, bytes_read, &addr);
	free(buf);
}

static void 
player_move(const ssp_segment_t* segment, waapp_t* app, UNUSED void* data)
{
	const net_udp_player_move_t* move = (net_udp_player_move_t*)segment->data;
	const vec2f_t* server_pos = &move->pos;
	const vec2f_t* client_pos;
	player_t* player = ght_get(&app->players, move->player_id);

	if (player)
	{
		client_pos = &player->core->pos;

		f32 dist = coregame_dist(client_pos, server_pos);

		if (dist > app->game.interp_threshold_dist)
		{
			player->core->interpolate = true;
			player->core->server_pos = *server_pos;
		}
		else
			player->core->pos = *server_pos;
		player->core->dir = move->dir;
	}
}

static void 
player_cursor(const ssp_segment_t* segment, waapp_t* app, UNUSED void* data)
{
	const net_udp_player_cursor_t* cursor = (net_udp_player_cursor_t*)segment->data;
	cg_player_t* player = ght_get(&app->game.players, cursor->player_id);
	if (player)
	{
		player->cursor = cursor->cursor_pos;
	}
}

static void 
player_shoot(const ssp_segment_t* segment, waapp_t* app, UNUSED void* data)
{
	const net_udp_player_shoot_t* shoot = (net_udp_player_shoot_t*)segment->data;
	cg_player_t* player = ght_get(&app->game.players, shoot->player_id);
	if (player)
		coregame_player_shoot(&app->game, player, shoot->shoot_dir);
}

static void 
player_health(const ssp_segment_t* segment, waapp_t* app, UNUSED void* data)
{
	const net_udp_player_health_t* health = (net_udp_player_health_t*)segment->data;
	player_t* player = ght_get(&app->players, health->player_id);
	if (player)
		player_set_health(player, health->health);
}

static void 
udp_pong(const ssp_segment_t* segment, waapp_t* app, UNUSED void* data)
{
	client_net_t* net = &app->net;
	struct timespec current_time;
	const net_udp_pingpong_t* pong = (const net_udp_pingpong_t*)segment->data;

	clock_gettime(CLOCK_MONOTONIC, &current_time);

	f64 elapsed_time = get_elapsed_time(&current_time, &pong->start_time);
	net->udp.latency = elapsed_time * 1000.0;
}

i32 
client_net_init(waapp_t* app, const char* ipaddr, u16 port)
{
	i32 ret;
	client_net_t* net = &app->net;
	ssp_segmap_callback_t callbacks[NET_SEGTYPES_LEN] = {0};
	callbacks[NET_TCP_SESSION_ID] = (ssp_segmap_callback_t)session_id;
	callbacks[NET_TCP_UDP_INFO] = (ssp_segmap_callback_t)udp_info;
	callbacks[NET_TCP_NEW_PLAYER] = (ssp_segmap_callback_t)new_player;
	callbacks[NET_TCP_DELETE_PLAYER] = (ssp_segmap_callback_t)delete_player;
	callbacks[NET_UDP_PLAYER_MOVE] = (ssp_segmap_callback_t)player_move;
	callbacks[NET_UDP_PLAYER_CURSOR] = (ssp_segmap_callback_t)player_cursor;
	callbacks[NET_UDP_PLAYER_SHOOT] = (ssp_segmap_callback_t)player_shoot;
	callbacks[NET_UDP_PLAYER_HEALTH] = (ssp_segmap_callback_t)player_health;
	callbacks[NET_UDP_PONG] = (ssp_segmap_callback_t)udp_pong;

	netdef_init(&net->def, &app->game, callbacks);
	net->def.ssp_state.user_data = app;

	array_init(&net->events, sizeof(fdevent_t), 4);

#ifdef _WIN32
	WSAStartup(MAKEWORD(2, 2), &net->wsa_data);
#endif

	ssp_tcp_sock_create(&net->tcp.sock, SSP_IPv4);
	if ((ret = ssp_tcp_connect(&net->tcp.sock, ipaddr, port)) == 0)
	{
		ssp_segbuff_init(&net->tcp.buf, 10, 0);
		net->udp.ipaddr = ipaddr;

		net_tcp_connect_t connect = {
			.username = "Test User"
		};

		ssp_segbuff_add(&net->tcp.buf, NET_TCP_CONNECT, sizeof(net_tcp_connect_t), &connect);
		ssp_tcp_send_segbuf(&net->tcp.sock, &net->tcp.buf);

#ifdef __linux__
		net->epfd = epoll_create1(EPOLL_CLOEXEC);
#endif

		client_net_add_fdevent(app, net->tcp.sock.sockfd, tcp_read, tcp_close, &net->tcp.sock);

		net->udp.fd = socket(AF_INET, SOCK_DGRAM, 0);
		net->udp.server.addr.sin_family = AF_INET;
		net->udp.server.addr.sin_addr.s_addr = inet_addr(ipaddr);
		net->udp.server.addr_len = sizeof(struct sockaddr_in);

		client_net_add_fdevent(app, net->udp.fd, udp_read, NULL, NULL);

		ssp_segbuff_init(&net->udp.buf, 10, SSP_FOOTER_BIT | SSP_SESSION_BIT | SSP_SEQUENCE_COUNT_BIT);

		client_net_poll(app, NULL, NULL);

#ifdef __linux__
		waapp_wayland_add_fdevent(app);
#endif
	}

	return ret;
}

#ifdef __linux__
// void
// client_net_poll(waapp_t* app, i32 timeout)
// {
// 	client_net_t* net = &app->net;
// 	struct epoll_event events[MAX_EVENTS];
// 	i32 nfds;
//
// 	do {
// 		if ((nfds = epoll_wait(net->epfd, events, MAX_EVENTS, timeout)) == -1)
// 		{
// 			perror("epoll_wait");
// 			return;
// 		}
//
// 		for (i32 i = 0; i < nfds; i++)
// 		{
// 			struct epoll_event* ev = events + i;
// 			fdevent_t* fdev = ev->data.ptr;
//
// 			if (ev->events & (EPOLLERR | EPOLLHUP))
// 			{
// 				fdev->close(app, fdev);
// 			}
// 			else if (ev->events & EPOLLIN)
// 			{
// 				fdev->read(app, fdev);
// 			}
// 		}
// 	} while(nfds && timeout == 0);
//
// 	client_net_get_stats(app);
// }

static inline void
ns_to_timespec(struct timespec* timespec, i64 ns)
{
	timespec->tv_sec = ns / 1e9;
	timespec->tv_nsec = ns % (i64)1e9;
}

static void 
handle_event(waapp_t* app, fdevent_t* fdev, u32 events)
{
	if (events & (EPOLLERR | EPOLLHUP))
	{
		fdev->close(app, fdev);
	}
	else if (events & EPOLLIN)
	{
		fdev->read(app, fdev);
	}
}

void
client_net_poll(waapp_t* app, struct timespec* prev_start_time, struct timespec* prev_end_time)
{
	i32 nfds;
	i64 prev_frame_time_ns;
	i64 elapsed_time_ns;
	i64 timeout_time_ns = 0;
	u32 errors = 0;
	client_net_t* net = &app->net;
	struct timespec timeout = {.tv_nsec = 1, .tv_sec = 0};
	struct timespec current_time;
	struct timespec* do_timeout = (prev_start_time == NULL) ? NULL : &timeout;
	struct epoll_event* event;
	struct epoll_event events[MAX_EVENTS];
	wa_state_t* state = wa_window_get_state(app->window);

	if (do_timeout && state->window.vsync == false && app->fps_limit)
	{
		prev_frame_time_ns = ((prev_end_time->tv_sec - prev_start_time->tv_sec) * 1e9) + 
							 (prev_end_time->tv_nsec - prev_start_time->tv_nsec);
		timeout_time_ns = app->fps_interval - prev_frame_time_ns;
		if (timeout_time_ns > 0)
			ns_to_timespec(&timeout, timeout_time_ns);
	}

	do {
		nfds = epoll_pwait2(net->epfd, events, MAX_EVENTS, do_timeout, NULL);
		if (nfds == -1)
		{
			if (errno == EINTR)
				continue;
			perror("epoll_pwait2");
			errors++;
			if (errors >= 3)
				break;
		}

		for (i32 i = 0; i < nfds; i++)
		{
			event = events + i;
			handle_event(app, event->data.ptr, event->events);
		}

		if (do_timeout && state->window.vsync == false && app->fps_limit)
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
		else 
			break;
	} while (nfds || timeout_time_ns > 0);

	client_net_get_stats(app);
}

#endif // __linux__

#ifdef _WIN32

static inline void
ns_to_timeval(struct timeval* timeval, i64 ns)
{
	timeval->tv_sec = ns / 1e9;
	timeval->tv_usec = (ns % (i64)1e9) / 1000;
}

void 
client_net_poll(waapp_t* app, struct timespec* prev_start_time, struct timespec* prev_end_time)
{
	client_net_t* net = &app->net;
	i64 timeout_time_ns = 0;
	i64 prev_frame_time_ns;
	i64 elapsed_time_ns;
	sock_t max_fd = 0;
	i32 ret;
	fdevent_t* events = (fdevent_t*)net->events.buf;
	struct timeval timeval = {0, 0};
	struct timeval* do_timeout = (prev_start_time == NULL) ? NULL : &timeval;
	struct timespec current_time;
	wa_state_t* state = wa_window_get_state(app->window);

	if (do_timeout && state->window.vsync == false && app->fps_limit)
	{
		prev_frame_time_ns = ((prev_end_time->tv_sec - prev_start_time->tv_sec) * 1e9) + 
							 (prev_end_time->tv_nsec - prev_start_time->tv_nsec);
		timeout_time_ns = app->fps_interval - prev_frame_time_ns;
		if (timeout_time_ns > 0)
			ns_to_timeval(&timeval, timeout_time_ns);
	}

	do {
		FD_ZERO(&net->read_fds);
		FD_ZERO(&net->execpt_fds);
		for (u32 i = 0; i < net->events.count; i++)
		{
			fdevent_t* fdev = events + i;
			FD_SET(fdev->fd, &net->read_fds);
			FD_SET(fdev->fd, &net->execpt_fds);
			if (fdev->fd > max_fd)
				max_fd = fdev->fd;
		}

		ret = select(max_fd + 1, &net->read_fds, NULL, &net->execpt_fds, do_timeout);
		if (ret == -1)
		{
			perror("select");
			return;
		}
		else if (ret > 0)
		{
			for (u32 i = 0; i < net->events.count; i++)
			{
				fdevent_t* fdev = events + i;
				if (FD_ISSET(fdev->fd, &net->execpt_fds))
				{
					if (fdev->close)
						fdev->close(app, fdev);
				}
				else if (FD_ISSET(fdev->fd, &net->read_fds))
				{
					fdev->read(app, fdev);
				}
			}
		}

		if (do_timeout && state->window.vsync == false && app->fps_limit)
		{
			clock_gettime(CLOCK_MONOTONIC, &current_time);
			elapsed_time_ns =	((current_time.tv_sec - prev_end_time->tv_sec) * 1e9) +
								(current_time.tv_nsec - prev_end_time->tv_nsec);
			memcpy(prev_end_time, &current_time, sizeof(struct timespec));
			timeout_time_ns -= elapsed_time_ns;
			if (timeout_time_ns > 0)
				ns_to_timeval(&timeval, timeout_time_ns);
			else
				timeval.tv_sec = timeval.tv_usec = 0;
		}
		else 
			break;
	} while(ret || timeout_time_ns > 0);

	client_net_get_stats(app);
}

// void
// client_net_poll(waapp_t* app, i32 timeout)
// {
// 	client_net_t* net = &app->net;
// 	sock_t max_fd = 0;
// 	i32 ret;
// 	fdevent_t* events = (fdevent_t*)net->events.buf;
// 	struct timeval timeval = {0, 0};
//
// 	do {
// 		FD_ZERO(&net->read_fds);
// 		FD_ZERO(&net->execpt_fds);
// 		for (u32 i = 0; i < net->events.count; i++)
// 		{
// 			fdevent_t* fdev = events + i;
// 			FD_SET(fdev->fd, &net->read_fds);
// 			FD_SET(fdev->fd, &net->execpt_fds);
// 			if (fdev->fd > max_fd)
// 				max_fd = fdev->fd;
// 		}
//
// 		ret = select(max_fd + 1, &net->read_fds, NULL, &net->execpt_fds, (timeout == -1) ? NULL : &timeval);
// 		if (ret == -1)
// 		{
// 			perror("select");
// 			return;
// 		}
// 		else if (ret > 0)
// 		{
// 			for (u32 i = 0; i < net->events.count; i++)
// 			{
// 				fdevent_t* fdev = events + i;
// 				if (FD_ISSET(fdev->fd, &net->execpt_fds))
// 				{
// 					if (fdev->close)
// 						fdev->close(app, fdev);
// 				}
// 				else if (FD_ISSET(fdev->fd, &net->read_fds))
// 				{
// 					fdev->read(app, fdev);
// 				}
// 			}
// 		}
// 	} while(ret > 0 && timeout == 0);
//
// 	client_net_get_stats(app);
// }
#endif // _WIN32

static void 
client_udp_send(waapp_t* app, ssp_packet_t* packet)
{
	client_net_t* net = &app->net;

	u32 packet_size = ssp_packet_size(packet);
	if (sendto(net->udp.fd, (void*)packet, packet_size, 0, (void*)&net->udp.server.addr, net->udp.server.addr_len) == -1)
	{
		perror("sendto");
	}

	net->udp.out.count++;
	net->udp.out.bytes += packet_size;

	free(packet);
}

static void
client_net_ping_server(waapp_t* app)
{
	ssp_packet_t* packet;
	net_udp_pingpong_t ping;
	clock_gettime(CLOCK_MONOTONIC, &ping.start_time);

	packet = ssp_insta_packet(&app->net.udp.buf, NET_UDP_PING, &ping, sizeof(net_udp_pingpong_t));
	client_udp_send(app, packet);
}

void 
client_net_get_stats(waapp_t* app)
{
	client_net_t* net = &app->net;

	clock_gettime(CLOCK_MONOTONIC, &net->udp.current_time);

	f64 elapsed_time = get_elapsed_time(&net->udp.current_time, &net->udp.inout_start_time);

	if (elapsed_time >= 1.0)
	{
		net->udp.in.last_count = net->udp.in.count;
		net->udp.in.last_bytes = net->udp.in.bytes;
		net->udp.in.count = 0;
		net->udp.in.bytes = 0;

		net->udp.out.last_count = net->udp.out.count;
		net->udp.out.last_bytes = net->udp.out.bytes;
		net->udp.out.count = 0;
		net->udp.out.bytes = 0;

		net->udp.inout_start_time = net->udp.current_time;

		client_net_ping_server(app);
	}
}

void
client_net_try_udp_flush(waapp_t* app)
{
	client_net_t* net = &app->net;

	// f64 elapsed_time = (net->udp.current_time.tv_sec - net->udp.send_start_time.tv_sec) + 
	// 					(net->udp.current_time.tv_nsec - net->udp.send_start_time.tv_nsec) / 1e9;
	f64 elapsed_time = get_elapsed_time(&net->udp.current_time, &net->udp.send_start_time);

	if (elapsed_time >= net->udp.interval)
	{
		ssp_packet_t* packet = ssp_serialize_packet(&net->udp.buf);
		if (packet)
		{
			client_udp_send(app, packet);
			mmframes_clear(&app->mmf);
		}
		net->udp.send_start_time = net->udp.current_time;
	}
}

void 
client_net_set_tickrate(waapp_t* app, f64 tickrate)
{
	app->net.udp.tickrate = tickrate;
	app->net.udp.interval = 1.0 / tickrate;
}

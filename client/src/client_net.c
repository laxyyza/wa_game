#define _GNU_SOURCE
#include "client_net.h"
#include "app.h"
#include "player.h"
#include <time.h>
#include "main_menu.h"
#include "state.h"
#include "gui/gui.h"
#include "game_net_events.h"
#include "cutils.h"

#ifdef __linux__
#include <sys/epoll.h>
#include "app_wayland.h"
#include <errno.h>
#include <fcntl.h>


#define NET_WOULDBLOCK EINPROGRESS
#endif

#ifdef _WIN32
#define WSA_ERROR_STR_MAX 256
char wsa_error_string[WSA_ERROR_STR_MAX];

#define NET_WOULDBLOCK WSAEWOULDBLOCK
#endif // _WIN32

#define BUFFER_SIZE 4096
#define MAX_EVENTS 4

f64 
get_elapsed_time(const struct timespec* current_time, const struct timespec* start_time)
{
	f64 elapsed_time =	(current_time->tv_sec - start_time->tv_sec) +
						(current_time->tv_nsec - start_time->tv_nsec) / 1e9;
	return elapsed_time;
}

#ifdef _WIN32
const char*
win32_err_string(i32 error_code)
{
	switch (error_code) 
	{
		case WSAEHOSTUNREACH:
			return "No route to host";
		case WSAEWOULDBLOCK:
			return "Resource temporarily unavailable";
		case WSAEISCONN:
			return "Already connected";
		default:
			return "Unknown error";
	}
}

const char* 
client_net_wsa_err_string(i32 error_code)
{
	char* msg;

	i32 ret = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		error_code,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
		(LPSTR)&msg,
		0, NULL
	);

	if (ret == 0)
		return win32_err_string(error_code);

	strncpy(wsa_error_string, msg, WSA_ERROR_STR_MAX);
	LocalFree(msg);

	u32 len = strlen(wsa_error_string);
	if (wsa_error_string[len - 2] == '\r')
		wsa_error_string[len - 2] = 0x00;

	return wsa_error_string;
}
#endif // _WIN32

static inline const char* 
client_net_errstr(i32 error_code)
{
#ifdef __linux__
	return strerror(error_code);
#elif _WIN32
	return client_net_wsa_err_string(error_code);
#endif
}

static inline i32
client_net_lasterr()
{
#ifdef __linux__
	return errno;
#elif _WIN32
	return WSAGetLastError();
#endif
}

static inline const char*
last_errstr()
{
	return client_net_errstr(client_net_lasterr());
}

static void 
client_net_fd_blocking(sock_t fd, bool block)
{
#ifdef __linux__
	i32 flags;

	if ((flags = fcntl(fd, F_GETFL, 0)) == -1)
	{
		perror("fcntl F_GETFL");
		return;
	}

	if (block)
		flags ^= O_NONBLOCK;
	else
		flags |= O_NONBLOCK;

	if (fcntl(fd, F_SETFL, flags) == -1)
		perror("fcntl: F_SETFL");
#endif // __linux__
#ifdef _WIN32	
	u_long mode = (block) ? 0 : 1;
	if (ioctlsocket(fd, FIONBIO, &mode) != 0)
	{
		errorf("Failed to set non-blocking mode: %d\n", WSAGetLastError());
	}
#endif
}

static void 
session_id(const ssp_segment_t* segment, waapp_t* app, UNUSED void* source_data)
{
	const net_tcp_sessionid_t* sessionid = (const net_tcp_sessionid_t*)segment->data;

	app->net.session_id = sessionid->session_id;
	app->net.player_id = sessionid->player_id;
	app->net.udp.buf.session_id = sessionid->session_id;

	debug("Got Session ID: %u, player ID: %u\n",
		sessionid->session_id, sessionid->player_id);
}

static void
client_net_on_connect(waapp_t* app)
{
	client_net_t* net = &app->net;
	waapp_main_menu_t* mm = (waapp_main_menu_t*)app->sm.states.main_menu.data;
	net->tcp.sock.connected = true;
	const char* username;

	if (app->save_username)
		username = app->save_username;
	else
		username = mm->sd.username;

	net_tcp_connect_t connect;
	memset(&connect, 9, sizeof(net_tcp_connect_t));
	strncpy(connect.username, username, PLAYER_NAME_MAX);

	ssp_segbuff_add(&net->tcp.buf, NET_TCP_CONNECT, sizeof(net_tcp_connect_t), &connect);
	ssp_tcp_send_segbuf(&net->tcp.sock, &net->tcp.buf);

	client_net_udp_init(app);

	if (app->save_username)
		free((void*)app->save_username);
}

void
client_net_del_fdevent(waapp_t* app, sock_t fd)
{
	client_net_t* net = &app->net;

	fdevent_t* events = (fdevent_t*)net->events.buf;

	for (u32 i = 0; i < net->events.count; i++)
	{
		if (events[i].fd == fd)
		{
			array_erase(&net->events, i);
			break;
		}
	}

#ifdef __linux__
	if (epoll_ctl(net->epfd, EPOLL_CTL_DEL, fd, NULL) == -1)
	{
		perror("epoll_ctl DEL");
		return;
	}

	// Update pointers.
	for (u32 i = 0; i < net->events.count; i++)
	{
		fdevent_t* fdev = events + i;

		struct epoll_event ev = {
			.data.ptr = fdev,
			.events = fdev->events
		};

		if (epoll_ctl(net->epfd, EPOLL_CTL_MOD, fdev->fd, &ev) == -1)
			perror("del_fdevent epoll_ctl MOD");
	}
#endif
}

fdevent_t*
client_net_add_fdevent(waapp_t* app, sock_t fd, 
					   fdevent_callback_t read, 
					   fdevent_callback_t close, 
					   fdevent_callback_t write, 
					   void* data)
{
	client_net_t* net = &app->net;
	fdevent_t* event = array_add_into(&net->events);
	event->fd = fd;
	event->read = read;
	event->close = close;
	event->write = write;
	event->err = NULL;
	event->data = data;

#ifdef __linux__
	event->events = EPOLLHUP;

	if (read)
		event->events |= EPOLLIN;
	if (write)
		event->events |= EPOLLOUT;

	struct epoll_event ev = {
		.data.ptr = event,
		.events = event->events
	};

	if (epoll_ctl(net->epfd, EPOLL_CTL_ADD, fd, &ev) == -1)
		perror("epoll_ctl ADD");
#endif
	return event;
}

#ifdef __linux__
static void 
client_net_fdevent_del_write(waapp_t* app, fdevent_t* fdev)
{
	if (fdev->events & EPOLLOUT)
	{
		fdev->events ^= EPOLLOUT;
		struct epoll_event ev = {
			.data.ptr = fdev,
			.events = fdev->events
		};
		if (epoll_ctl(app->net.epfd, EPOLL_CTL_MOD, fdev->fd, &ev) == -1)
			perror("epoll_ctl MOD");
	}
	fdev->write = NULL;
}
#endif // __linux__

#ifdef _WIN32
static void 
client_net_fdevent_del_write(UNUSED waapp_t* app, fdevent_t* fdev)
{
	fdev->write = NULL;
}
#endif // _WIN32

static void
tcp_close(waapp_t* app, fdevent_t* fdev)
{
	ssp_tcp_sock_t* sock = fdev->data;

	client_net_del_fdevent(app, fdev->fd);
	ssp_tcp_sock_close(sock);
}

static void
tcp_read(waapp_t* app, fdevent_t* fdev)
{
	void* buf = malloc(BUFFER_SIZE);
	i64 bytes_read;

	if ((bytes_read = recv(fdev->fd, buf, BUFFER_SIZE, 0)) == -1)
		perror("tcp_recv");
	else
		ssp_parse_buf(&app->net.def.ssp_state, &app->net.tcp.buf, buf, bytes_read, NULL);
	free(buf);
}

static void
tcp_error(waapp_t* app, fdevent_t* fdev, i32 error_code)
{
	client_net_t* net = &app->net;
	ssp_tcp_sock_t* s = &net->tcp.sock;
	waapp_main_menu_t* mm;
	const char* errstr;

	if (s->connected == false)
	{
		mm = app->sm.states.main_menu.data;

		errstr = client_net_errstr(error_code);
		snprintf(mm->state, MM_STATE_STRING_MAX, "FAILED: %s (%d)", errstr, error_code);

		client_net_del_fdevent(app, fdev->fd);
		ssp_tcp_sock_close(s);
		ssp_tcp_sock_create(s, s->addr.domain);
	}
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
udp_close(waapp_t* app)
{
	client_net_t* net = &app->net;

	if (net->udp.fdev == NULL)
		return;

	client_net_del_fdevent(app, net->udp.fd);
#ifdef __linux__
	close(net->udp.fd);
#elif _WIN32
	closesocket(net->udp.fd);
#endif
	memset(&net->udp, 0, sizeof(client_udp_t));
}

void
client_net_udp_init(waapp_t* app)
{
	client_net_t* net = &app->net;
	const char* ipaddr = net->tcp.sock.ipstr;

	net->udp.fd = socket(AF_INET, SOCK_DGRAM, 0);
	net->udp.server.addr.sin_family = AF_INET;
	net->udp.server.addr.sin_addr.s_addr = inet_addr(ipaddr);
	net->udp.server.addr_len = sizeof(struct sockaddr_in);
	net->udp.ipaddr = ipaddr;

	net->udp.fdev = client_net_add_fdevent(app, net->udp.fd, udp_read, NULL, NULL, NULL);
}

static void
tcp_write_connect(waapp_t* app, fdevent_t* fdev)
{
	i32 err;
	socklen_t len = sizeof(i32);
	getsockopt(fdev->fd, SOL_SOCKET, SO_ERROR, (char*)&err, &len);

	debug("tcp_connect: %s (%d)\n", client_net_errstr(err), err);
	if (err)
		return;

	client_net_fd_blocking(fdev->fd, true);

	client_net_on_connect(app);
	client_net_fdevent_del_write(app, fdev);
}

static void
udp_info(const ssp_segment_t* segment, waapp_t* app, UNUSED void* source_data)
{
	net_tcp_udp_info_t* info = (net_tcp_udp_info_t*)segment->data;
	info("UDP Server Port: %u\n", info->port);
	app->net.udp.server.addr.sin_port = htons(info->port);
	client_net_set_tickrate(app, info->tickrate);

	app->net.udp.port = info->port;

	ssp_segbuff_init(&app->net.udp.buf, 10, info->ssp_flags);

	waapp_state_switch(app, &app->sm.states.game);
}

static void
server_stats(const ssp_segment_t* segment, waapp_t* app, UNUSED void* source_data)
{
	const server_stats_t* stats = (const server_stats_t*)segment->data;

	memcpy(&app->net.server_stats, stats, sizeof(server_stats_t));
}

static void 
udp_pong(const ssp_segment_t* segment, waapp_t* app, UNUSED void* data)
{
	client_net_t* net = &app->net;
	struct timespec current_time;
	const net_udp_pingpong_t* pong = (const net_udp_pingpong_t*)segment->data;
	net_udp_player_ping_t* player_ping = mmframes_alloc(&app->mmf, sizeof(net_udp_player_ping_t));

	clock_gettime(CLOCK_MONOTONIC, &current_time);

	f64 elapsed_time = get_elapsed_time(&current_time, &pong->start_time);
	app->game->player->core->stats.ping = player_ping->ms = net->udp.latency = elapsed_time * 1000.0;

	ssp_segbuff_add(&net->udp.buf, NET_UDP_PLAYER_PING, sizeof(net_udp_player_ping_t), player_ping);
}

i32 
client_net_connect(waapp_t* app, const char* ipaddr, u16 port)
{
	i32 ret;
	client_net_t* net = &app->net;

	if ((ret = ssp_tcp_connect(&net->tcp.sock, ipaddr, port)) == 0)
	{
		net->udp.ipaddr = ipaddr;

		net_tcp_connect_t connect = {
			.username = "Test User"
		};

		ssp_segbuff_add(&net->tcp.buf, NET_TCP_CONNECT, sizeof(net_tcp_connect_t), &connect);
		ssp_tcp_send_segbuf(&net->tcp.sock, &net->tcp.buf);

		client_net_add_fdevent(app, net->tcp.sock.sockfd, tcp_read, tcp_close, NULL, &net->tcp.sock);

		client_net_poll(app, NULL, NULL);

	}
	return ret;
}

static bool
client_net_parse_address(waapp_t* app, const char* addr)
{
	client_net_t* net = &app->net;
	ssp_tcp_sock_t* sock = &net->tcp.sock;
	char* ip_addr = sock->ipstr;
	u16 port = DEFAULT_PORT;
	bool ret = true;

	// TODO: Support adding port into address string.
	strncpy(ip_addr, addr, INET6_ADDRSTRLEN - 1);

	sock->addr.port = port;
	sock->addr.addr_len = sizeof(struct sockaddr_in);
	sock->addr.sockaddr.in.sin_family = AF_INET;
	sock->addr.sockaddr.in.sin_addr.s_addr = inet_addr(ip_addr);
	sock->addr.sockaddr.in.sin_port = htons(port);

	return ret;
}

const char* 
client_net_async_connect(waapp_t* app, const char* addr)
{
	const char* ret;
	i32 res;
	client_net_t* net = &app->net;
	ssp_tcp_sock_t* sock = &net->tcp.sock;
	fdevent_callback_t write_cb = NULL;

	if (client_net_parse_address(app, addr) == false)
		return "Invalid Address";

	client_net_fd_blocking(sock->sockfd, false);

	res = connect(sock->sockfd, (struct sockaddr*)&sock->addr.sockaddr, sock->addr.addr_len);
	if (res == -1)
	{
		i32 err = client_net_lasterr();
		if (err == NET_WOULDBLOCK)
		{
			ret = "Connecting...";
			write_cb = tcp_write_connect;
		}
		else
		{
			error("async_connect: '%s' (%d)\n", client_net_errstr(err), err);
			return "Connect FAILED.";
		}
	}

	net->tcp.fdev = client_net_add_fdevent(app, sock->sockfd, tcp_read, tcp_close, write_cb, sock);
	net->tcp.fdev->err = tcp_error;

	if (write_cb == NULL)
	{
		client_net_on_connect(app);
		ret = "Connected.";
	}

	return ret;
}

static void 
do_reconnect(UNUSED const ssp_segment_t* segment, waapp_t* app, UNUSED void* data)
{
	cg_player_t* player = app->game->player->core;
	client_net_disconnect(app);
	const char* ret = client_net_async_connect(app, app->net.tcp.sock.ipstr);
	printf("do reconnect: %s\n", ret);
	app->save_username = strndup(player->username, PLAYER_NAME_MAX);
	app->game->player = NULL;

	coregame_free_player(&app->game->cg, player);
}

i32 
client_net_init(waapp_t* app)
{
	i32 ret = 0;
	client_net_t* net = &app->net;
	ssp_segmap_callback_t callbacks[NET_SEGTYPES_LEN] = {0};
	callbacks[NET_TCP_SESSION_ID] = (ssp_segmap_callback_t)session_id;
	callbacks[NET_TCP_UDP_INFO] = (ssp_segmap_callback_t)udp_info;
	callbacks[NET_TCP_NEW_PLAYER] = (ssp_segmap_callback_t)game_new_player;
	callbacks[NET_TCP_DELETE_PLAYER] = (ssp_segmap_callback_t)game_delete_player;
	callbacks[NET_UDP_PLAYER_MOVE] = (ssp_segmap_callback_t)game_player_move;
	callbacks[NET_UDP_PLAYER_CURSOR] = (ssp_segmap_callback_t)game_player_cursor;
	callbacks[NET_UDP_PLAYER_SHOOT] = (ssp_segmap_callback_t)game_player_shoot;
	callbacks[NET_UDP_PLAYER_HEALTH] = (ssp_segmap_callback_t)game_player_health;
	callbacks[NET_UDP_PONG] = (ssp_segmap_callback_t)udp_pong;
	callbacks[NET_UDP_PLAYER_DIED] = (ssp_segmap_callback_t)game_player_died;
	callbacks[NET_UDP_PLAYER_STATS] = (ssp_segmap_callback_t)game_player_stats;
	callbacks[NET_UDP_PLAYER_PING] = (ssp_segmap_callback_t)game_player_ping;
	callbacks[NET_TCP_CG_MAP] = (ssp_segmap_callback_t)game_cg_map;
	callbacks[NET_TCP_SERVER_SHUTDOWN] = (ssp_segmap_callback_t)game_server_shutdown;
	callbacks[NET_UDP_SERVER_STATS] = (ssp_segmap_callback_t)server_stats;
	callbacks[NET_TCP_CHAT_MSG] = (ssp_segmap_callback_t)game_chat_msg;
	callbacks[NET_UDP_DO_RECONNECT] = (ssp_segmap_callback_t)do_reconnect;
	callbacks[NET_TCP_GUN_SPEC] = (ssp_segmap_callback_t)game_gun_spec;
	callbacks[NET_UDP_PLAYER_GUN_ID] = (ssp_segmap_callback_t)game_player_gun_id;

	netdef_init(&net->def, NULL, callbacks);
	net->def.ssp_state.user_data = app;

	array_init(&net->events, sizeof(fdevent_t), 4);

#ifdef _WIN32
	WSAStartup(MAKEWORD(2, 2), &net->wsa_data);
#endif

	ssp_tcp_sock_create(&net->tcp.sock, SSP_IPv4);

#ifdef __linux__
	net->epfd = epoll_create1(EPOLL_CLOEXEC);
	waapp_wayland_add_fdevent(app);
#endif

	ssp_segbuff_init(&net->tcp.buf, 10, 0);

	return ret;
}

void 
client_net_disconnect(waapp_t* app)
{
	client_net_t* net = &app->net;

	if (net->tcp.fdev)
	{
		tcp_close(app, net->tcp.fdev);
		net->tcp.fdev = NULL;
		ssp_tcp_sock_create(&net->tcp.sock, net->tcp.sock.addr.domain);
	}
	udp_close(app);
}

#ifdef __linux__

static inline void
ns_to_timespec(struct timespec* timespec, i64 ns)
{
	timespec->tv_sec = ns / 1e9;
	timespec->tv_nsec = ns % (i64)1e9;
}

static void 
handle_event(waapp_t* app, fdevent_t* fdev, u32 events)
{
	if (events & EPOLLERR)
	{
		i32 err = 0;
		socklen_t len = sizeof(i32);
		getsockopt(fdev->fd, SOL_SOCKET, SO_ERROR, &err, &len);

		error("Socket (%d) error: \"%s\" (%d)\n", fdev->fd, strerror(err), err);
		if (fdev->err)
			fdev->err(app, fdev, err);
		else
			fdev->close(app, fdev);
		return;
	}
	if (events & EPOLLHUP)
	{
		infof("EPOLLHUP\n");
		fdev->close(app, fdev);
	}
	if (events & EPOLLIN)
		fdev->read(app, fdev);
	if (events & EPOLLOUT)
		fdev->write(app, fdev);
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

	if (prev_start_time)
	{
		prev_frame_time_ns = ((prev_end_time->tv_sec - prev_start_time->tv_sec) * 1e9) + 
							 (prev_end_time->tv_nsec - prev_start_time->tv_nsec);
		app->frame_time = prev_frame_time_ns / 1e6;
	}

	if (do_timeout && state->window.vsync == false && app->fps_limit)
	{
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

	if (app->net.events.count == 0)
	{
		wa_window_poll_timeout(app->window, 0);
		return;
	}

	if (prev_start_time)
	{
		prev_frame_time_ns = ((prev_end_time->tv_sec - prev_start_time->tv_sec) * 1e9) + 
							 (prev_end_time->tv_nsec - prev_start_time->tv_nsec);
		app->frame_time = prev_frame_time_ns / 1e6;
	}

	if (do_timeout && state->window.vsync == false && app->fps_limit)
	{
		timeout_time_ns = app->fps_interval - prev_frame_time_ns;
		if (timeout_time_ns > 0)
			ns_to_timeval(&timeval, timeout_time_ns);
	}

	do {
		FD_ZERO(&net->read_fds);
		FD_ZERO(&net->execpt_fds);
		FD_ZERO(&net->write_fds);
		for (u32 i = 0; i < net->events.count; i++)
		{
			fdevent_t* fdev = events + i;
			if (fdev->write)
				FD_SET(fdev->fd, &net->write_fds);
			else
				FD_SET(fdev->fd, &net->read_fds);
			FD_SET(fdev->fd, &net->execpt_fds);
			if (fdev->fd > max_fd)
				max_fd = fdev->fd;
		}

		ret = select(max_fd + 1, &net->read_fds, &net->write_fds, &net->execpt_fds, do_timeout);
		if (ret == -1)
		{
			errorf("select error: %s\n", last_errstr());
			return;
		}
		else if (ret > 0)
		{
			for (u32 i = 0; i < net->events.count; i++)
			{
				fdevent_t* fdev = events + i;
				if (fdev->write && FD_ISSET(fdev->fd, &net->write_fds))
					fdev->write(app, fdev);
				else if (FD_ISSET(fdev->fd, &net->execpt_fds))
				{
					i32 err = 0;
					socklen_t len = sizeof(i32);
					getsockopt(fdev->fd, SOL_SOCKET, SO_ERROR, (char*)&err, &len);

					if (fdev->err)
						fdev->err(app, fdev, err);
					else if (fdev->close)
						fdev->close(app, fdev);
				}
				else if (FD_ISSET(fdev->fd, &net->read_fds))
					fdev->read(app, fdev);
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
	wa_window_poll_timeout(app->window, 0);
}

#endif // _WIN32

static void 
client_udp_send(waapp_t* app, ssp_packet_t* packet)
{
	client_net_t* net = &app->net;

	if (sendto(net->udp.fd, packet->buf, packet->size, 0, 
			  (void*)&net->udp.server.addr, net->udp.server.addr_len) == -1)
	{
		perror("sendto");
	}

	net->udp.out.count++;
	net->udp.out.bytes += packet->size;

	ssp_packet_free(packet);
}

static void
client_net_ping_server(waapp_t* app)
{
	if (app->net.udp.port == 0)
		return;

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

	if (net->tcp.sock.connected == false)
		return;

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

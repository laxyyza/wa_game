#ifndef _SERVER_H_
#define _SERVER_H_

#include "server_common.h"
#include "client.h"
#include "event.h"
#include "netdef.h"
#include "mmframes.h"

#define MAX_EVENTS 8
#define FRAMETIMES_LEN 128
#define FRAMETIME_LEN 63
#define MAX_SPAWN_POINTS 8

typedef struct server
{
	i32 udp_fd;
	ssp_tcp_sock_t tcp_sock;
	i32 epfd;
	u16 port;
	u16 udp_port;
	ght_t clients;
	struct epoll_event events[MAX_EVENTS];
	netdef_t netdef;
	coregame_t game;

	vec2f_t spawn_points[MAX_SPAWN_POINTS];
	u32 spawn_idx;

	mmframes_t mmf;
	bool running;
	f64		tickrate;
	f64		interval;
	i64		interval_ns; // Interval Nanoseconds

	i64		highest_frametime;
	char	highest_frametime_str[FRAMETIME_LEN];
} server_t;

i32 server_init(server_t* server, i32 argc, const char** argv);
void server_run(server_t* server);
void server_cleanup(server_t* server);

#endif // _SERVER_H_

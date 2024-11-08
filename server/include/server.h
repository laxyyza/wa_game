#ifndef _SERVER_H_
#define _SERVER_H_

#define _GNU_SOURCE
#include "server_common.h"
#include "client.h"
#include "event.h"
#include "netdef.h"
#include "mmframes.h"

#include <sys/random.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <sys/timerfd.h>
#include <sys/signalfd.h>

#define MAX_EVENTS 8
#define FRAMETIMES_LEN 128
#define FRAMETIME_LEN 63
#define SSP_FLAGS (SSP_SESSION_BIT | SSP_SEQUENCE_COUNT_BIT)

typedef struct server
{
	i32 udp_fd;
	ssp_tcp_sock_t tcp_sock;
	i32 epfd;
	i32 signalfd;
	i32 timerfd;
	u16 port;
	u16 udp_port;
	ght_t clients;
	struct epoll_event ep_events[MAX_EVENTS];
	netdef_t netdef;
	coregame_t game;

	array_t spawn_points;
	u32 spawn_idx;

	mmframes_t mmf;
	bool running;
	f64		tickrate;
	f64		interval;
	i64		interval_ns; // Interval Nanoseconds

	i64		highest_frametime;
	char	highest_frametime_str[FRAMETIME_LEN];

	struct {
		event_t* head;
		event_t* tail;
	} events;
	const char* cgmap_path;
	cg_map_t*	disk_map;
	u32			disk_map_size;

	struct timespec start_time;
	struct timespec end_time;
	struct timespec prev_time;

	f64 routine_time;
	f64 client_timeout_threshold;
} server_t;

i32 server_init(server_t* server, i32 argc, char* const* argv);
void server_run(server_t* server);
void server_cleanup(server_t* server);
void server_close_client(server_t* server, client_t* client);

void server_timerfd_timeout(server_t* server, event_t* event);
void signalfd_read(server_t* server, event_t* event);
bool server_verify_session(u32 session_id, server_t* server, udp_addr_t* source_data, void** new_source, ssp_segbuff_t** segbuf);
void server_read_udp_packet(server_t* server, event_t* event);
void server_handle_new_connection(server_t* server, UNUSED event_t* event);
void event_signalfd_close(server_t* server, UNUSED event_t* ev);
vec2f_t server_next_spawn(server_t* server);

#endif // _SERVER_H_

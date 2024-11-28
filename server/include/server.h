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
#define SSP_FLAGS (SSP_SESSION_BIT)

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

	char	highest_frametime_str[FRAMETIME_LEN];

	struct {
		event_t* head;
		event_t* tail;
	} events;
	const char* cgmap_path;
	cg_disk_map_t*	disk_map;
	u32			disk_map_size;

	nano_timer_t timer;
	hr_time_t prev_time;
	f64 current_time;

	f64 routine_time;
	f64 client_timeout_threshold;

	u64 tick_count;
	u64 tick_time_total;
	server_stats_t stats;
	f64 last_stat_update;
	bool send_stats;
	bool reset_stats;

	ssp_segbuf_t segbuf;

	array_t bullet_create_events;
} server_t;

i32 server_init(server_t* server, i32 argc, char* const* argv);
void server_run(server_t* server);
void server_cleanup(server_t* server);
void server_close_client(server_t* server, client_t* client);

void server_timerfd_timeout(server_t* server, event_t* event);
void signalfd_read(server_t* server, event_t* event);
bool server_verify_session(u32 session_id, server_t* server, udp_addr_t* source_data, void** new_source, ssp_segbuf_t** segbuf);
void server_read_udp_packet(server_t* server, event_t* event);
void server_handle_new_connection(server_t* server, UNUSED event_t* event);
void event_signalfd_close(server_t* server, UNUSED event_t* ev);
vec2f_t server_next_spawn(server_t* server);
void server_add_data_all_udp_clients(server_t* server, u8 type, const void* data, u16 size, u32 ignore_player_id);
void server_add_data_all_udp_clients_i(server_t* server, u8 type, const void* data, u16 size, u32 ignore_player_id);

#endif // _SERVER_H_

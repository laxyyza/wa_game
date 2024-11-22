#ifndef _CLIENT_NET_H_
#define _CLIENT_NET_H_

#define _GNU_SOURCE
#include "netdef.h"
#include "ssp.h"
#include "ssp_tcp.h"
#include "time.h"
#include "array.h"

#ifdef _WIN32
#include <windows.h>

#define MAX_SOCKETS 8
#endif

typedef struct waapp waapp_t;
typedef struct fdevent fdevent_t;

typedef void (*fdevent_callback_t)(waapp_t* app, fdevent_t* fdevent);
typedef void (*fdevent_err_callback_t)(waapp_t* app, fdevent_t* fdevent, i32 error_code);

typedef struct fdevent
{
	sock_t fd;
	void* data;
	u32 events;
	fdevent_callback_t read;
	fdevent_callback_t write;
	fdevent_callback_t close;
	fdevent_err_callback_t err;
} fdevent_t;

typedef struct 
{
	sock_t fd;

	f64 tickrate;
	f64 interval;
	f64 latency;

	udp_addr_t server;

	ssp_segbuf_t buf;

	hr_time_t inout_start_time;
	hr_time_t send_start_time;

	struct {
		u32 count;
		u32 last_count;

		u64 bytes;
		u64 last_bytes;
	} in;

	struct {
		u32 count;
		u32 last_count;

		u64 bytes;
		u64 last_bytes;
	} out;

	const char* ipaddr;
	u16 port;

	fdevent_t* fdev;
} client_udp_t;

typedef struct 
{
#ifdef __linux__
	i32 epfd;
#endif
	u32 session_id;
	u32 player_id;

	netdef_t def;

	struct {
		ssp_tcp_sock_t	sock;
		ssp_segbuf_t	buf;
		fdevent_t*		fdev;
	} tcp;

	client_udp_t udp;

	array_t events;
	server_stats_t server_stats;

#ifdef _WIN32
	fd_set read_fds;
	fd_set execpt_fds;
	fd_set write_fds;
	WSADATA wsa_data;
#endif
} client_net_t;

i32 client_net_init(waapp_t* app);
void client_net_disconnect(waapp_t* app);
const char* client_net_async_connect(waapp_t* app, const char* addr);
void client_net_poll(waapp_t* app);
// void client_net_poll(waapp_t* app, i32 timeout);
void client_net_try_udp_flush(waapp_t* app);
void client_net_get_stats(waapp_t* app);
void client_net_set_tickrate(waapp_t* app, f64 tickrate);
fdevent_t* client_net_add_fdevent(waapp_t* app, sock_t fd, 
							fdevent_callback_t read, 
							fdevent_callback_t close, 
							fdevent_callback_t write, 
							void* data);
void client_net_udp_init(waapp_t* app);

#endif // _CLIENT_NET_H_

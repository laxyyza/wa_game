#ifndef _CLIENT_NET_H_
#define _CLIENT_NET_H_

#define _GNU_SOURCE
#include "netdef.h"
#include "ssp.h"
#include "ssp_tcp.h"
#include "time.h"

typedef struct waapp waapp_t;
typedef struct fdevent fdevent_t;

typedef void (*fdevent_callback_t)(waapp_t* app, fdevent_t* fdevent);

typedef struct fdevent
{
	i32 fd;
	void* data;
	fdevent_callback_t read;
	fdevent_callback_t close;
} fdevent_t;

typedef struct 
{
	i32 fd;

	f64 tickrate;
	f64 interval;

	udp_addr_t server;

	ssp_segbuff_t buf;

	struct timespec current_time;
	struct timespec inout_start_time;
	struct timespec send_start_time;

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
} client_udp_t;

typedef struct 
{
	i32 epfd;
	u32 session_id;
	u32 player_id;

	netdef_t def;

	struct {
		ssp_tcp_sock_t	sock;
		ssp_segbuff_t	buf;
	} tcp;

	client_udp_t udp;
} client_net_t;

i32 client_net_init(waapp_t* app, const char* ipaddr, u16 port, f64 tickrate);
void client_net_poll(waapp_t* app, i32 timeout);
void client_net_try_udp_flush(waapp_t* app);
void client_net_get_stats(waapp_t* app);
void client_net_set_tickrate(waapp_t* app, f64 tickrate);

#endif // _CLIENT_NET_H_

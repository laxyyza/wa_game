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
	i32 epfd;
	i32 udp_fd;
	u32 session_id;
	u32 player_id;
	netdef_t def;
	udp_addr_t server_udp;
	ssp_tcp_sock_t tcp;
	ssp_segbuff_t segbuf;
	ssp_segbuff_t udp_buf;

	struct timespec start_time;
	struct timespec current_time;
	u32 count;
	u64 bytes;
} client_net_t;

i32 client_net_init(waapp_t* app, const char* ipaddr, u16 port);
void client_net_poll(waapp_t* app, i32 timeout);

#endif // _CLIENT_NET_H_

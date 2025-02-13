#ifndef _CLIENT_H_
#define _CLIENT_H_

#include "server_common.h"
#include "netdef.h"

typedef struct server server_t;

typedef struct 
{
	u32				session_id;
	cg_player_t*	player;
	ssp_tcp_sock_t	tcp_sock;
	ssp_io_t		tcp_io;
	ssp_io_t		udp_io;
	udp_addr_t		udp;
	char*			og_username;
	bool			udp_connected;
	bool			want_stats;
	bool			bot;
	f64				last_packet_time;
} client_t;

client_t* accept_client(server_t* server);
i64 client_send(server_t* server, client_t* client, ssp_packet_t* packet);

#endif // _CLIENT_H_

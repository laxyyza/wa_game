#ifndef _CLIENT_H_
#define _CLIENT_H_

#include "server_common.h"

typedef struct server server_t;

typedef struct 
{
	u32 session_id;
	cg_player_t* player;
	ssp_tcp_sock_t tcp_sock;
	ssp_segbuff_t segbuf;
} client_t;

client_t* accept_client(server_t* server);

#endif // _CLIENT_H_

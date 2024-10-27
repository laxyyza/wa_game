#ifndef _CLIENT_H_
#define _CLIENT_H_

#include "server_common.h"

typedef struct server server_t;

typedef struct 
{
	ssp_tcp_sock_t tcp_sock;
	u32 game_id;
} client_t;

client_t* accept_client(server_t* server);

#endif // _CLIENT_H_

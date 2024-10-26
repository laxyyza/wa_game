#ifndef _CLIENT_H_
#define _CLIENT_H_

#include "server_common.h"

typedef struct server server_t;

typedef struct 
{
	i32 tcp_fd;
	u32 game_id;
	char ipaddr[INET_ADDRSTRLEN];
} client_t;

client_t* accept_client(server_t* server);

#endif // _CLIENT_H_

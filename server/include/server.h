#ifndef _SERVER_H_
#define _SERVER_H_

#include <stdio.h>
#include <ght.h>
#include <int.h>
#include <coregame.h>
#include "client.h"

typedef struct
{
	i32 fd;
	i32 epfd;
	ght_t clients;
	bool running;
} server_t;

i32 server_init(server_t* server, i32 argc, const char** argv);
void server_run(server_t* server);
void server_cleanup(server_t* server);

#endif // _SERVER_H_

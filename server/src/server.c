#include "server.h"

i32 
server_init(UNUSED server_t* server, UNUSED i32 argc, UNUSED const char** argv)
{
	printf("server_init()...\n");
	return 0;
}

void 
server_run(UNUSED server_t* server)
{
	printf("server_run()...\n");
}

void 
server_cleanup(UNUSED server_t* server)
{
	printf("server_cleanup()...\n");
}

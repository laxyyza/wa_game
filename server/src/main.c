#include "server.h"

static server_t server = {0};

i32
main(i32 argc, const char** argv)
{
	if (server_init(&server, argc, argv) == -1)
		return -1;

	server_run(&server);

	server_cleanup(&server);

	return 0;
}

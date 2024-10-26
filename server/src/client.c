#include "client.h"
#include "server.h"

client_t* 
accept_client(server_t* server)
{
	client_t* client = calloc(1, sizeof(client_t));
	struct sockaddr_in addr;
	socklen_t len = server->addr_len;

	if ((client->tcp_fd = accept(server->tcp_fd, (struct sockaddr*)&addr, &len)) == -1)
	{
		perror("accept");
		free(client);
		return NULL;
	}

	inet_ntop(AF_INET, &addr.sin_addr.s_addr, client->ipaddr, len);

	printf("New client '%s' (%d) connected.\n", client->ipaddr, client->tcp_fd);

	return client;
}

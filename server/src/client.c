#include "client.h"
#include "server.h"
#include <sys/random.h>

client_t* 
accept_client(server_t* server)
{
	client_t* client = calloc(1, sizeof(client_t));
	client->tcp_sock.addr.addr_len = server->tcp_sock.addr.addr_len;

	if ((client->tcp_sock.sockfd = accept(server->tcp_sock.sockfd, (struct sockaddr*)&client->tcp_sock.addr.sockaddr, &client->tcp_sock.addr.addr_len)) == -1)
	{
		perror("accept");
		free(client);
		return NULL;
	}

	inet_ntop(AF_INET, &client->tcp_sock.addr.sockaddr.in.sin_addr.s_addr, client->tcp_sock.ipstr, client->tcp_sock.addr.addr_len);

	printf("New client '%s' (%d) connected.\n", client->tcp_sock.ipstr, client->tcp_sock.sockfd);

	ssp_segbuff_init(&client->segbuf, 10);

	getrandom(&client->session_id, sizeof(u32), 0);

	ght_insert(&server->clients, client->session_id, client);

	return client;
}

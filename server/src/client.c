#include "client.h"
#include "server.h"
#include <sys/random.h>

#define COMPRESSION_THRESHOLD 1000
#define COMPRESSION_LEVEL 5

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

	ssp_segbuff_init(&client->tcp_buf, 10, 0);
	client->tcp_buf.compression.auto_do = true;
	client->tcp_buf.compression.threshold = COMPRESSION_THRESHOLD; // Only do compression over this.
	client->tcp_buf.compression.level = COMPRESSION_LEVEL;
	ssp_segbuff_init(&client->udp_buf, 10, SSP_FLAGS);

	getrandom(&client->session_id, sizeof(u32), 0);

	ght_insert(&server->clients, client->session_id, client);

	printf("Client (%s) (fd:%d) connected.\t(%zu connected clients)\n", 
			client->tcp_sock.ipstr, client->tcp_sock.sockfd, server->clients.count);

	return client;
}

i64 
client_send(server_t* server, client_t* client, ssp_packet_t* packet)
{
	i64 bytes_sent;
	if ((bytes_sent = sendto(server->udp_fd, packet->buf, packet->size, 0, 
							(struct sockaddr*)&client->udp.addr, client->udp.addr_len)) == -1)
	{
		perror("sendto");
	}
	ssp_packet_free(packet);

	return bytes_sent;
}

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

	ssp_segbuff_init(&client->tcp_buf, 10, 0);
	ssp_segbuff_init(&client->udp_buf, 10, SSP_FOOTER_BIT | SSP_SESSION_BIT | SSP_SEQUENCE_COUNT_BIT);

	getrandom(&client->session_id, sizeof(u32), 0);

	ght_insert(&server->clients, client->session_id, client);

	return client;
}

i64 
client_send(server_t* server, client_t* client, ssp_packet_t* packet)
{
	i64 bytes_sent;
	u32 packet_size = ssp_packet_size(packet);
	if ((bytes_sent = sendto(server->udp_fd, packet, packet_size, 0, (struct sockaddr*)&client->udp.addr, client->udp.addr_len)) == -1)
	{
		perror("sendto");
	}
	free(packet);

	return bytes_sent;
}

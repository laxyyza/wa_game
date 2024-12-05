#include "client.h"
#include "server.h"
#include <sys/random.h>

#define TX_COMPRESSION_THRESHOLD 1000
#define UDP_TX_COMPRESSION_THRESHOLD 1400
#define TX_COMPRESSION_LEVEL 9
#define UDP_TX_COMPRESSION_LEVEL 2

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

	ssp_io_init(&client->tcp_io, &server->netdef.ssp_ctx, 0);
	client->tcp_io.tx.compression.auto_do = true;
	client->tcp_io.tx.compression.threshold = TX_COMPRESSION_THRESHOLD; // Only do tx.compression over this.
	client->tcp_io.tx.compression.level = TX_COMPRESSION_LEVEL;

	ssp_io_init(&client->udp_io, &server->netdef.ssp_ctx, SSP_FLAGS);
	client->udp_io.tx.compression.auto_do = true;
	client->udp_io.tx.compression.threshold = UDP_TX_COMPRESSION_THRESHOLD; // Only do tx.compression over this.
	client->udp_io.tx.compression.level = UDP_TX_COMPRESSION_LEVEL;

	getrandom(&client->session_id, sizeof(u32), 0);
	client->udp_io.session_id = client->session_id;

	ght_insert(&server->clients, client->session_id, client);

	client->last_packet_time = server->timer.start_time_s;

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

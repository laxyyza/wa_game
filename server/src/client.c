#include "client.h"
#include "server.h"
#include <sys/random.h>

#define COMPRESSION_THRESHOLD 1000
#define UDP_COMPRESSION_THRESHOLD 1400
#define COMPRESSION_LEVEL 9
#define UDP_COMPRESSION_LEVEL 2

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

	ssp_segbuf_init(&client->tcp_buf, 10, 0);
	client->tcp_buf.compression.auto_do = true;
	client->tcp_buf.compression.threshold = COMPRESSION_THRESHOLD; // Only do compression over this.
	client->tcp_buf.compression.level = COMPRESSION_LEVEL;

	ssp_segbuf_init(&client->udp_buf, 20, SSP_FLAGS);
	client->udp_buf.compression.auto_do = true;
	client->udp_buf.compression.threshold = UDP_COMPRESSION_THRESHOLD; // Only do compression over this.
	client->udp_buf.compression.level = UDP_COMPRESSION_LEVEL;

	getrandom(&client->session_id, sizeof(u32), 0);
	client->udp_buf.session_id = client->session_id;

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

#include "server.h"
#include <sys/random.h>

#define PORT 8080

static i32
server_init_tcp(server_t* server)
{
	i32 ret;

	ret = ssp_tcp_server(&server->tcp_sock, SSP_IPv4, server->port);

	return ret;
}

static i32
server_init_udp(server_t* server)
{
	if ((server->udp_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		perror("socket UDP");
		return -1;
	}

	server->tcp_sock.addr.sockaddr.in.sin_port = htons(server->port + 1);
	if (bind(server->udp_fd, (struct sockaddr*)&server->tcp_sock.addr.sockaddr, server->tcp_sock.addr.addr_len) == -1)
	{
		perror("bind UDP");
		return -1;
	}


	return 0;
}

static void
read_client(server_t* server, event_t* event)
{
	client_t* client = event->data;
	char buffer[1024] = {0};

	ssize_t bytes_read;

	if ((bytes_read = recv(event->fd, buffer, 1024, 0)) == -1)
	{
		perror("recv");
		server_close_event(server, event);
	}
	else if (bytes_read == 0)
	{
		server_close_event(server, event);
	}
	else
	{
		ssp_parse_buf(&server->netdef.ssp_state, buffer, bytes_read, client);
	}
}

static void
close_client(UNUSED server_t* server, event_t* event)
{
	client_t* client = event->data;

	ssp_tcp_sock_close(&client->tcp_sock);
	printf("Client '%s' closed.\n", client->tcp_sock.ipstr);

	free(client);
}

static void
handle_new_connection(server_t* server, UNUSED event_t* event)
{
	client_t* client = accept_client(server);
	if (client == NULL)
		return;

	server_add_event(server, client->tcp_sock.sockfd, client, read_client, close_client);
}

static void 
read_udp_packet(UNUSED server_t* server, event_t* event)
{
	char buffer[1024] = {0};

	if (recvfrom(event->fd, buffer, 1024, 0, NULL, NULL) == -1)\
	{
		perror("recvfrom");
	}

	printf("UDP Packet: '%s'\n", buffer);
}

static i32
server_init_epoll(server_t* server)
{
	if ((server->epfd = epoll_create1(EPOLL_CLOEXEC)) == -1)
	{
		perror("epoll_create1");
		return -1;
	}

	server_add_event(server, server->tcp_sock.sockfd, NULL, handle_new_connection, NULL);
	server_add_event(server, server->udp_fd, NULL, read_udp_packet, NULL);

	return 0;
}

// static i32
// server_init_coregame(server_t* server)
// {
// 	return 0;
// }

static void 
client_tcp_connect(const ssp_segment_t* segment, UNUSED server_t* server, client_t* client)
{
	net_tcp_sessionid_t sessionid;
	const net_tcp_connect_t* connect = (net_tcp_connect_t*)segment->data;

	getrandom(&sessionid.session_id, sizeof(u32), 0);
	getrandom(&sessionid.player_id, sizeof(u32), 0);
	client->session_id = sessionid.session_id;
	client->player_id = sessionid.player_id;

	printf("Client '%s' (%s) got %u for session ID.\n", connect->username, client->tcp_sock.ipstr, sessionid.session_id);

	ssp_segbuff_add(&client->segbuf, NET_TCP_SESSION_ID, sizeof(net_tcp_sessionid_t), &sessionid);
	ssp_tcp_send_segbuf(&client->tcp_sock, &client->segbuf);
}

static void
server_init_netdef(server_t* server)
{
	ssp_segmap_callback_t callbacks[NET_SEGTYPES_LEN] = {0};
	callbacks[NET_TCP_CONNECT] = (ssp_segmap_callback_t)client_tcp_connect;

	netdef_init(&server->netdef, NULL, callbacks);
	server->netdef.ssp_state.user_data = server;
}

i32 
server_init(server_t* server, UNUSED i32 argc, UNUSED const char** argv)
{
	printf("server_init()...\n");

	server->port = PORT;

	if (server_init_tcp(server) == -1)
		goto err;
	if (server_init_udp(server) == -1)
		goto err;
	if (server_init_epoll(server) == -1)
		goto err;
	server_init_netdef(server);
	// if (server_init_coregame(server) == -1)
	// 	goto err;

	server->running = true;

	return 0;
err:
	server_cleanup(server);
	return -1;
}

void 
server_run(server_t* server)
{
	i32 nfds;
	printf("server_run()...\n");

	while (server->running)
	{
		if ((nfds = epoll_wait(server->epfd, server->events, MAX_EVENTS, -1)) == -1)
		{
			perror("epoll_wait");
			server->running = false;
		}

		for (i32 i = 0; i < nfds; i++)
		{
			struct epoll_event* event = server->events + i;
			server_handle_event(server, event->data.ptr, event->events);
		}
	}
}

void 
server_cleanup(UNUSED server_t* server)
{
	printf("server_cleanup()...\n");
}

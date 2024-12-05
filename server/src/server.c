#include "server.h"
#include "server_game.h"

#define RECV_BUFFER_SIZE 4096

void
server_add_data_all_udp_clients(server_t* server, u8 type, const void* data, u16 size, 
								u32 ignore_player_id)
{
	ght_t* clients = &server->clients;

	GHT_FOREACH(client_t* client, clients, 
	{
		if (client->player && client->player->id != ignore_player_id)
			ssp_segbuf_add(&client->udp_buf, type, size, data);
	});
}

void
server_add_data_all_udp_clients_i(server_t* server, u8 type, const void* data, u16 size, 
								u32 ignore_player_id)
{
	ght_t* clients = &server->clients;

	GHT_FOREACH(client_t* client, clients, 
	{
		if (client->player && client->player->id != ignore_player_id)
			ssp_segbuf_add_i(&client->udp_buf, type, size, data);
	});
}

vec2f_t 
server_next_spawn(server_t* server)
{
	vec2f_t ret;
	const cg_runtime_map_t* map = server->game.map;
	const cg_runtime_cell_t* spawn_cell = *(const cg_runtime_cell_t**)array_idx(&server->spawn_points, server->spawn_idx);

	ret = vec2f(
		spawn_cell->pos.x * map->grid_size, 
		spawn_cell->pos.y * map->grid_size
	);
	server->spawn_idx++;
	if (server->spawn_idx >= server->spawn_points.count)
		server->spawn_idx = 0;
	return ret;
}

static void
server_routine_checks(server_t* server)
{
	ght_t* clients = &server->clients;
	if (clients->count == 0)
		return;

	GHT_FOREACH(client_t* client, clients, {
		f64 time_elapsed = server->current_time - client->last_packet_time;

		if (time_elapsed > server->client_timeout_threshold)
		{
			printf("Didn't receive a packet from client (%s) for more than %.1fs. ", 
					client->tcp_sock.ipstr, server->client_timeout_threshold);
			server_close_client(server, client);
		}
	});
}

void
server_timerfd_timeout(server_t* server, event_t* event)
{
	u64 expirations;

	if (read(event->fd, &expirations, sizeof(u64)) == -1)
	{
		perror("timerfd read");
		return;
	}

	server_routine_checks(server);
}

static void
read_client(server_t* server, event_t* event)
{
	client_t* client = event->data;
	void* buf = malloc(RECV_BUFFER_SIZE);
	i32 ret;

	i64 bytes_read;

	if ((bytes_read = recv(event->fd, buf, RECV_BUFFER_SIZE, 0)) == -1)
	{
		perror("recv");
		server_close_event(server, event);
	}
	else if (bytes_read == 0)
		server_close_event(server, event);
	else
	{
		ret = ssp_parse_buf(&server->netdef.ssp_ctx, &client->tcp_buf, buf, bytes_read, client, server->current_time);
		if (ret == SSP_FAILED)
		{
			printf("Client (%s) sent invalid packet. Closing client.\n",
				client->tcp_sock.ipstr);
			server_close_event(server, event);
		}
		client->last_packet_time = server->timer.start_time_s;
	}

	free(buf);
}

void
server_close_client(server_t* server, client_t* client)
{
	u32 player_id = 0;

	if (server->running == false && client->player)
	{
		ssp_segbuf_add(&client->tcp_buf, NET_TCP_SERVER_SHUTDOWN, 0, NULL);
		ssp_tcp_send_segbuf(&client->tcp_sock, &client->tcp_buf);
	}

	ssp_tcp_sock_close(&client->tcp_sock);
	printf("Client (%s) (fd:%d) closed.\t(%zu connected clients)\n", 
			client->tcp_sock.ipstr, client->tcp_sock.sockfd, server->clients.count - 1);

	if (client->player)
	{
		player_id = client->player->id;
		coregame_free_player(&server->game, client->player);
	}

	ssp_segbuf_destroy(&client->udp_buf);
	ssp_segbuf_destroy(&client->tcp_buf);
	if (client->og_username)
		free(client->og_username);
	ght_del(&server->clients, client->session_id);

	if (player_id)
		broadcast_delete_player(server, player_id);
}

static void
event_close_client(server_t* server, event_t* event)
{
	server_close_client(server, event->data);

	server->stats.tcp_connections = server->clients.count;
	server->stats.players = server->game.players.count;
}

void
server_handle_new_connection(server_t* server, UNUSED event_t* event)
{
	client_t* client = accept_client(server);
	if (client == NULL)
		return;

	server_add_event(server, client->tcp_sock.sockfd, client, read_client, event_close_client);
	server->stats.tcp_connections = server->clients.count;
	server->stats.players = server->game.players.count;
}

static void
set_udp_info(udp_addr_t* info)
{
	inet_ntop(AF_INET, &info->addr.sin_addr, info->ipaddr, INET6_ADDRSTRLEN);
	info->port = ntohs(info->addr.sin_port);
}

void 
server_read_udp_packet(server_t* server, event_t* event)
{
	void* buf = calloc(1, RECV_BUFFER_SIZE);
	i64 bytes_read;
	i32 ret;
	f64 timestamp_s;
	hr_time_t current_time;
	udp_addr_t info = {
		.addr_len = sizeof(struct sockaddr_in),
	};

	nano_gettime(&current_time);
	timestamp_s = nano_time_s(&current_time);

	if ((bytes_read = recvfrom(event->fd, buf, RECV_BUFFER_SIZE, 0, &info.addr, &info.addr_len)) == -1)
	{
		perror("recvfrom");
		free(buf);
		return;
	}

	set_udp_info(&info);
	server->stats.udp_pps_in++;
	if ((server->stats.udp_pps_in_bytes += bytes_read) > server->stats.udp_pps_in_bytes_highest)
		server->stats.udp_pps_in_bytes_highest = server->stats.udp_pps_in_bytes;

	ret = ssp_parse_buf(&server->netdef.ssp_ctx, NULL, buf, bytes_read, &info, timestamp_s);
	if (ret == SSP_FAILED)
		printf("Invalid UDP packet (%zu bytes) from %s:%u.\n", bytes_read, info.ipaddr, info.port);

	if (ret != SSP_BUFFERED)
		free(buf);
}

static void
server_ask_client_reconnect(server_t* server, udp_addr_t* client)
{
	ssp_packet_t* packet = ssp_insta_packet(&server->segbuf, NET_UDP_DO_RECONNECT, NULL, 0);
	
	if (sendto(server->udp_fd, packet->buf, packet->size, 0, 
			(struct sockaddr*)&client->addr, client->addr_len) == -1)
	{
		perror("server_ask_client_reconnect sendto");
		goto free_packet;
	}

free_packet:
	ssp_packet_free(packet);
}

bool 
server_verify_session(u32 session_id, server_t* server, udp_addr_t* source_data, void** new_source, ssp_segbuf_t** segbuf)
{
	client_t* client;

	client = ght_get(&server->clients, session_id);
	if (client == NULL)
	{
		printf("No client with session ID: %u\n", session_id);
		server_ask_client_reconnect(server, source_data);
		return false;
	}

	if (client->udp_connected)
	{
		if (client->udp.addr.sin_addr.s_addr != source_data->addr.sin_addr.s_addr)
		{
			printf("UDP \"connected\" client IP Address dones't match from new packet. (og: %s != new: %s)...\n",
					client->udp.ipaddr, source_data->ipaddr);
		}

		if (client->udp.addr.sin_port != source_data->addr.sin_port)
		{
			printf("UDP \"connected\" client source port dones't match from new packet. (og: %u != new: %u)...\n",
					client->udp.port, source_data->port);
		}
	}
	else if (client->tcp_sock.addr.sockaddr.in.sin_addr.s_addr != source_data->addr.sin_addr.s_addr)
	{
		printf("Client TCP IP Address (%s) != to UDP Address (%s)!\n",
				client->tcp_sock.ipstr, source_data->ipaddr);
		return false;
	}
	else
	{
		memcpy(&client->udp, source_data, sizeof(udp_addr_t));
		client->udp_connected = true;
	}
	*new_source = client;
	*segbuf = &client->udp_buf;
	client->last_packet_time = server->timer.start_time_s;

	return true;
}

void
signalfd_read(server_t* server, event_t* event)
{
	struct signalfd_siginfo siginfo;
	const i64 size = sizeof(struct signalfd_siginfo);
	i32 sig;

	if (read(event->fd, &siginfo, size) != size)
	{
		perror("signalfd_read");
		server->running = false;
		return;
	}
	sig = siginfo.ssi_signo;

	printf("\nSignal recieved: %d (%s)\n", sig, strsignal(sig));

	switch (sig)
	{
		case SIGINT:
		case SIGTERM:
			server->running = false;
			break;
		default:
			break;
	}
}

static void
signalfd_close(server_t* server)
{
	if (server->signalfd >= 0)
		return;

	if (close(server->signalfd) == -1)
		perror("signalfd_close");
}

void
event_signalfd_close(server_t* server, UNUSED event_t* ev)
{
	signalfd_close(server);
}

static inline void 
server_buffer_packet(server_t* server, const ssp_packet_t* packet, client_t* client)
{
	struct iovec* iov = mmframes_alloc(&server->mmf, sizeof(struct iovec));
	iov->iov_base = packet->buf;
	iov->iov_len = packet->size;

	server->total_tx_size += packet->size;

	struct mmsghdr* msgvec = array_add_into(&server->tx_msgs);
	msgvec->msg_hdr.msg_name = &client->udp.addr;
	msgvec->msg_hdr.msg_namelen = client->udp.addr_len;
	msgvec->msg_hdr.msg_iov = iov;
	msgvec->msg_hdr.msg_iovlen = 1;
	msgvec->msg_hdr.msg_control = NULL;
	msgvec->msg_hdr.msg_controllen = 0;
	msgvec->msg_hdr.msg_flags = 0;
	msgvec->msg_len = 0;

	array_add_voidp(&server->packet_tx_buf, (void*)packet);
}

static inline void
server_prepare_udp_client(server_t* server, client_t* client)
{
	if (client->udp_connected == false)
		return;

	if (server->send_stats && client->want_stats)
	{
		server->stats.udp_pps_out++;
		server->stats.udp_pps_out_bytes += ssp_segbuf_serialized_size(&client->udp_buf, NULL) + sizeof(server_stats_t);
		if (server->stats.udp_pps_out_bytes > server->stats.udp_pps_out_bytes_highest)
			server->stats.udp_pps_out_bytes_highest = server->stats.udp_pps_out_bytes;

		server->stats.tx.rto = client->udp_buf.rto;
		server->stats.tx.total_packets = client->udp_buf.out_total_packets;

		server->stats.rx.lost = client->udp_buf.sliding_window.lost_packets;
		server->stats.rx.dropped = client->udp_buf.in_dropped_packets;
		server->stats.rx.total_packets = client->udp_buf.in_total_packets;

		ssp_segbuf_add(&client->udp_buf, NET_UDP_SERVER_STATS, sizeof(server_stats_t), &server->stats);
	}

	ssp_packet_t* packet = ssp_serialize_packet(&client->udp_buf);
	if (packet)
	{
		packet->timestamp = server->current_time;

		if (!(server->send_stats && client->want_stats))
		{
			server->stats.udp_pps_out++;
			server->stats.udp_pps_out_bytes += packet->size;
		}

		server_buffer_packet(server, packet, client);
	}

	while ((packet = ssp_segbuf_get_resend_packet(&client->udp_buf, server->current_time)))
	{
		if (!(server->send_stats && client->want_stats))
		{
			server->stats.udp_pps_out++;
			server->stats.udp_pps_out_bytes += packet->size;
		}

		server_buffer_packet(server, packet, client);
	}

	ssp_parse_sliding_window(&server->netdef.ssp_ctx, &client->udp_buf, client);
}

static inline void
server_sendmmsg(server_t* server)
{
	if (server->packet_tx_buf.count == 0)
		return;

	struct mmsghdr* msgvec = (struct mmsghdr*)server->tx_msgs.buf;

	i64 ret = sendmmsg(server->udp_fd, msgvec, server->tx_msgs.count, 0);
	if (ret == -1)
	{
		perror("sendmmsg");
	}

	for (u32 i = 0; i < server->packet_tx_buf.count; i++)
	{
		ssp_packet_t* packet = ((ssp_packet_t**)server->packet_tx_buf.buf)[i];
		ssp_packet_free(packet);
	}

	array_clear(&server->packet_tx_buf, false);
	array_clear(&server->tx_msgs, false);
	server->total_tx_size = 0;
}

static void 
server_flush_udp_clients(server_t* server)
{
	ght_t* clients = &server->clients;

	GHT_FOREACH(client_t* client, clients, 
	{
		server_prepare_udp_client(server, client);
	});

	server_sendmmsg(server);

	if (server->send_stats)
	{
		server->send_stats = false;
		server->stats.udp_pps_in = 0;
		server->stats.udp_pps_in_bytes = 0;

		server->stats.udp_pps_out = 0;
		server->stats.udp_pps_out_bytes = 0;
	}
}

static inline void
ns_to_timespec(struct timespec* timespec, i64 ns)
{
	timespec->tv_sec = ns / 1e9;
	timespec->tv_nsec = ns % (i64)1e9;
}

// static void 
// format_ns(char* buf, u64 max, i64 ns)
// {
// 	if (ns < 1e6)
// 		snprintf(buf, max, "%ld ns", ns);
// 	else
// 		snprintf(buf, max, "%.3f ms", (f64)ns / 1e6);
// }

static void
server_poll(server_t* server)
{
	i32 nfds;
	i64 prev_frame_time_ns;
	i64 elapsed_time_ns;
	i64 timeout_time_ns = 0;
	u32 errors = 0;
	struct timespec timeout = {0};
	hr_time_t current_time;
	struct timespec* do_timeout = (server->clients.count == 0) ? NULL : &timeout;
	struct epoll_event* event;


	f64 current_time_s = server->timer.start_time_s;
	f64 time_elapsed = current_time_s - server->last_stat_update;
	
	if (time_elapsed >= 1.0)
	{
		server->send_stats = true;

		server->last_stat_update = current_time_s;
	}

	if (do_timeout)
	{
		prev_frame_time_ns = server->timer.elapsed_time_ns;
		server->stats.tick_time = prev_frame_time_ns;
		server->tick_time_total += prev_frame_time_ns;
		server->stats.tick_time_avg = server->tick_time_total / server->tick_count;
		if (prev_frame_time_ns > server->stats.tick_time_highest)
		{
			server->stats.tick_time_highest = prev_frame_time_ns;
			// format_ns(server->highest_frametime_str, FRAMETIME_LEN, prev_frame_time_ns);
		}
		timeout_time_ns = server->interval_ns - prev_frame_time_ns;
		if (timeout_time_ns > 0)
			ns_to_timespec(&timeout, timeout_time_ns);
	}

	do {
		do_timeout = (server->clients.count == 0) ? NULL : &timeout;

		nfds = epoll_pwait2(server->epfd, server->ep_events, MAX_EVENTS, do_timeout, NULL);
		if (nfds == -1)
		{
			if (errno == EINTR)
				continue;
			perror("epoll_pwait2");
			errors++;
			if (errors >= 3)
			{
				server->running = false;
				break;
			}
		}

		for (i32 i = 0; i < nfds; i++)
		{
			event = server->ep_events + i;
			server_handle_event(server, event->data.ptr, event->events);
		}

		if (do_timeout)
		{
			nano_gettime(&current_time);
			elapsed_time_ns = nano_time_diff_ns(&server->timer.end_time, &current_time);
			timeout_time_ns -= elapsed_time_ns;
			memcpy(&server->timer.end_time, &current_time, sizeof(hr_time_t));
			if (timeout_time_ns > 0)
				ns_to_timespec(&timeout, timeout_time_ns);
			else
				timeout.tv_sec = timeout.tv_nsec = 0;
		}
	} while ((nfds || timeout_time_ns > 0) && server->running);
}

void 
server_run(server_t* server)
{
	if (server->running)
	{
		printf("Server is up & running!\n\t");
		printf("Tick rate: %.1f     (%fms interval).\n\t",
				server->tickrate, server->interval * 1000.0);
		printf("TCP port:  %u\n\t", server->port);
		printf("UDP port:  %u\n\t", server->udp_port);
		printf("Connect to TCP port.\n\n");
	}

	server->last_stat_update = server->timer.start_time_s;

	while (server->running)
	{
		server_poll(server);

		nano_start_time(&server->timer);
		server->current_time = server->timer.start_time_s;
		server->netdef.ssp_ctx.current_time = server->current_time;

		coregame_update(&server->game);
		server_flush_udp_clients(server);
		mmframes_clear(&server->mmf);
		server->tick_count++;

		nano_end_time(&server->timer);
	}
}

static void
server_cleanup_clients(server_t* server)
{
	ght_t* clients = &server->clients;

	GHT_FOREACH(client_t* client, clients, {
		if (client->player)
		{
			ssp_segbuf_add(&client->tcp_buf, NET_TCP_SERVER_SHUTDOWN, 0, NULL);
			ssp_tcp_send_segbuf(&client->tcp_sock, &client->tcp_buf);
		}
		server_close_client(server, client);
	});

	ght_destroy(&server->clients);
}

void 
server_cleanup(server_t* server)
{
	array_del(&server->packet_tx_buf);
	server_close_all_events(server);
	server_cleanup_clients(server);
	ssp_tcp_sock_close(&server->tcp_sock);
	signalfd_close(server);
	ssp_segbuf_destroy(&server->segbuf);
	free(server->disk_map);

	if (server->timerfd > 0 && close(server->timerfd) == -1)
		perror("close timerfd");

	if (server->udp_fd > 0 && close(server->udp_fd) == -1)
		perror("close udp fd");

	if (server->epfd > 0 && close(server->epfd) == -1)
		perror("close epoll");

	coregame_cleanup(&server->game);
	array_del(&server->spawn_points);
	mmframes_free(&server->mmf);
	netdef_destroy(&server->netdef);
}

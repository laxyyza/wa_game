#define _GNU_SOURCE
#include "client_net.h"
#include "app.h"
#include <sys/epoll.h>
#include "player.h"
#include <time.h>

#define BUFFER_SIZE 4096
#define MAX_EVENTS 2

static void 
session_id(const ssp_segment_t* segment, waapp_t* app, UNUSED void* source_data)
{
	const net_tcp_sessionid_t* sessionid = (const net_tcp_sessionid_t*)segment->data;

	app->net.session_id = sessionid->session_id;
	app->net.player_id = sessionid->player_id;
	app->net.udp_buf.session_id = sessionid->session_id;

	printf("Got Session ID: %u, player ID: %u\n",
		sessionid->session_id, sessionid->player_id);
}

static void 
add_fdevent(waapp_t* app, i32 fd, fdevent_callback_t read, fdevent_callback_t close, void* data)
{
	client_net_t* net = &app->net;
	fdevent_t* event = calloc(1, sizeof(fdevent_t));
	event->fd = fd;
	event->read = read;
	event->close = close;
	event->data = data;

	struct epoll_event ev = {
		.data.ptr = event,
		.events = EPOLLIN | EPOLLHUP
	};

	if (epoll_ctl(net->epfd, EPOLL_CTL_ADD, fd, &ev) == -1)
		perror("epoll_ctl ADD");
}

static void
tcp_close(waapp_t* app, fdevent_t* fdev)
{
	ssp_tcp_sock_t* sock = fdev->data;

	if (epoll_ctl(app->net.epfd, EPOLL_CTL_DEL, fdev->fd, NULL) == -1)
		perror("epoll_ctl DEL");

	ssp_tcp_sock_close(sock);
	free(fdev);

	printf("TCP Socket Disconnected.\n");

}

static void
tcp_read(waapp_t* app, fdevent_t* fdev)
{
	void* buf = malloc(BUFFER_SIZE);
	i64 bytes_read;

	if ((bytes_read = recv(fdev->fd, buf, BUFFER_SIZE, 0)) == -1)
	{
		perror("recv");
		fdev->close(app, fdev);
	}
	else if (bytes_read == 0)
	{
		fdev->close(app, fdev);
	}
	else
	{
		ssp_parse_buf(&app->net.def.ssp_state, buf, bytes_read, NULL);
	}
	free(buf);
}

static void 
new_player(const ssp_segment_t* segment, waapp_t* app, UNUSED void* user_data)
{
	const cg_player_t* new_player = (const cg_player_t*)segment->data;

	cg_player_t* cg_player = malloc(sizeof(cg_player_t));
	memcpy(cg_player, new_player, sizeof(cg_player_t));

	player_t* player = player_new_from(app, cg_player);

	if (cg_player->id == app->net.player_id)
		app->player = player;

	coregame_add_player_from(&app->game, cg_player);
}

static void 
delete_player(const ssp_segment_t* segment, waapp_t* app, UNUSED void* user_data)
{
	const net_tcp_delete_player_t* del_player = (const net_tcp_delete_player_t*)segment->data;

	cg_player_t* player = ght_get(&app->game.players, del_player->player_id);
	if (player)
	{
		coregame_free_player(&app->game, player);
	}
}

static void
udp_info(const ssp_segment_t* segment, waapp_t* app, UNUSED void* source_data)
{
	net_tcp_udp_info_t* info = (net_tcp_udp_info_t*)segment->data;
	printf("UDP Server Port: %u\n", info->port);
	app->net.server_udp.addr.sin_port = htons(info->port);
}

static void
udp_read(waapp_t* app, fdevent_t* fdev)
{
	client_net_t* net = &app->net;

	net->count++;

	void* buf = malloc(BUFFER_SIZE);
	i64 bytes_read;
	udp_addr_t addr = {
		.addr_len = sizeof(struct sockaddr_in)
	};

	if ((bytes_read = recvfrom(fdev->fd, buf, BUFFER_SIZE, 0, (struct sockaddr*)&addr.addr, &addr.addr_len)) == -1)
	{
		perror("recvfrom");
		return;
	}

	net->bytes += bytes_read;

	ssp_parse_buf(&app->net.def.ssp_state, buf, bytes_read, &addr);
	free(buf);
}

static void 
player_move(const ssp_segment_t* segment, waapp_t* app, UNUSED void* data)
{
	const net_udp_player_move_t* move = (net_udp_player_move_t*)segment->data;
	cg_player_t* player = ght_get(&app->game.players, move->player_id);
	if (player && app->player->core->id != move->player_id)
	{
		player->pos = move->pos;
		player->dir = move->dir;
	}
}

static void 
player_cursor(const ssp_segment_t* segment, waapp_t* app, UNUSED void* data)
{
	const net_udp_player_cursor_t* cursor = (net_udp_player_cursor_t*)segment->data;
	cg_player_t* player = ght_get(&app->game.players, cursor->player_id);
	if (player)
	{
		player->cursor = cursor->cursor_pos;
	}
}

static void 
player_shoot(const ssp_segment_t* segment, waapp_t* app, UNUSED void* data)
{
	const net_udp_player_shoot_t* shoot = (net_udp_player_shoot_t*)segment->data;
	cg_player_t* player = ght_get(&app->game.players, shoot->player_id);
	if (player)
	{
		cg_projectile_t* proj = coregame_player_shoot(&app->game, player, shoot->shoot_dir);
		projectile_new(app, proj);
	}
}

i32 
client_net_init(waapp_t* app, const char* ipaddr, u16 port)
{
	i32 ret;
	client_net_t* net = &app->net;
	ssp_segmap_callback_t callbacks[NET_SEGTYPES_LEN] = {0};
	callbacks[NET_TCP_SESSION_ID] = (ssp_segmap_callback_t)session_id;
	callbacks[NET_TCP_UDP_INFO] = (ssp_segmap_callback_t)udp_info;
	callbacks[NET_TCP_NEW_PLAYER] = (ssp_segmap_callback_t)new_player;
	callbacks[NET_TCP_DELETE_PLAYER] = (ssp_segmap_callback_t)delete_player;
	callbacks[NET_UDP_PLAYER_MOVE] = (ssp_segmap_callback_t)player_move;
	callbacks[NET_UDP_PLAYER_CURSOR] = (ssp_segmap_callback_t)player_cursor;
	callbacks[NET_UDP_PLAYER_SHOOT] = (ssp_segmap_callback_t)player_shoot;

	netdef_init(&net->def, &app->game, callbacks);
	net->def.ssp_state.user_data = app;

	ssp_tcp_sock_create(&net->tcp, SSP_IPv4);
	if ((ret = ssp_tcp_connect(&net->tcp, ipaddr, port)) == 0)
	{
		ssp_segbuff_init(&net->segbuf, 10, 0);

		net_tcp_connect_t connect = {
			.username = "Test User"
		};

		ssp_segbuff_add(&net->segbuf, NET_TCP_CONNECT, sizeof(net_tcp_connect_t), &connect);
		ssp_tcp_send_segbuf(&net->tcp, &net->segbuf);

		net->epfd = epoll_create1(EPOLL_CLOEXEC);

		add_fdevent(app, net->tcp.sockfd, tcp_read, tcp_close, &net->tcp);

		net->udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
		net->server_udp.addr.sin_family = AF_INET;
		net->server_udp.addr.sin_addr.s_addr = inet_addr(ipaddr);
		net->server_udp.addr_len = sizeof(struct sockaddr_in);

		add_fdevent(app, net->udp_fd, udp_read, NULL, NULL);

		ssp_segbuff_init(&net->udp_buf, 10, SSP_FOOTER_BIT | SSP_SESSION_BIT);

		client_net_poll(app, -1);
	}

	return ret;
}

void
client_net_poll(waapp_t* app, i32 timeout)
{
	client_net_t* net = &app->net;
	struct epoll_event events[MAX_EVENTS];
	i32 nfds;

	do {
		if ((nfds = epoll_wait(net->epfd, events, MAX_EVENTS, timeout)) == -1)
		{
			perror("epoll_wait");
			return;
		}

		for (i32 i = 0; i < nfds; i++)
		{
			struct epoll_event* ev = events + i;
			fdevent_t* fdev = ev->data.ptr;

			if (ev->events & (EPOLLERR | EPOLLHUP))
			{
				fdev->close(app, fdev);
			}
			else if (ev->events & EPOLLIN)
			{
				fdev->read(app, fdev);
			}
		}
	} while(nfds && timeout == 0);
}

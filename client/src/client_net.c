#include "client_net.h"
#include "app.h"
#include <sys/epoll.h>
#include "player.h"

#define BUFFER_SIZE 1024
#define MAX_EVENTS 2

static void 
session_id(const ssp_segment_t* segment, waapp_t* app, UNUSED void* source_data)
{
	const net_tcp_sessionid_t* sessionid = (const net_tcp_sessionid_t*)segment->data;

	app->net.session_id = sessionid->session_id;
	app->net.player_id = sessionid->player_id;

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

i32 
client_net_init(waapp_t* app, const char* ipaddr, u16 port)
{
	i32 ret;
	client_net_t* net = &app->net;
	ssp_segmap_callback_t callbacks[NET_SEGTYPES_LEN] = {0};
	callbacks[NET_TCP_SESSION_ID] = (ssp_segmap_callback_t)session_id;
	callbacks[NET_TCP_NEW_PLAYER] = (ssp_segmap_callback_t)new_player;
	callbacks[NET_TCP_DELETE_PLAYER] = (ssp_segmap_callback_t)delete_player;

	netdef_init(&net->def, &app->game, callbacks);
	net->def.ssp_state.user_data = app;

	ssp_tcp_sock_create(&net->tcp, SSP_IPv4);
	if ((ret = ssp_tcp_connect(&net->tcp, ipaddr, port)) == 0)
	{
		ssp_segbuff_init(&net->segbuf, 10);

		net_tcp_connect_t connect = {
			.username = "Test User"
		};

		ssp_segbuff_add(&net->segbuf, NET_TCP_CONNECT, sizeof(net_tcp_connect_t), &connect);
		ssp_tcp_send_segbuf(&net->tcp, &net->segbuf);

		net->epfd = epoll_create1(EPOLL_CLOEXEC);

		add_fdevent(app, net->tcp.sockfd, tcp_read, tcp_close, &net->tcp);

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
}

#include "server.h"

static void 
server_ep_add_event(server_t* server, event_t* event)
{
	struct epoll_event ev = {
		.data.ptr = event,
		.events = EPOLLIN | EPOLLRDHUP
	};
	if (epoll_ctl(server->epfd, EPOLL_CTL_ADD, event->fd, &ev) == -1)
	{
		perror("epoll_ctl ADD");
		return;
	}
}

static void 
server_ep_del_event(server_t* server, event_t* event)
{
	if (epoll_ctl(server->epfd, EPOLL_CTL_DEL, event->fd, NULL) == -1)
		perror("epoll_ctl DEL");
}

void
server_close_all_events(server_t* server)
{
	event_t* event = server->events.head;
	event_t* next;

	while (event)
	{
		next = event->next;
		server_close_event(server, event);
		event = next;
	}
}

void 
server_add_event(server_t* server, i32 fd, void* data, event_read_t read, event_close_t close)
{
	event_t* event = calloc(1, sizeof(event_t));
	event->fd = fd;
	event->data = data;
	event->read = read;
	event->close = close;

	server_ep_add_event(server, event);

	if (server->events.head == NULL)
	{
		server->events.head = event;
		server->events.tail = event;
		return;
	}

	server->events.tail->next = event;
	event->prev = server->events.tail;
	server->events.tail = event;
}

void 
server_close_event(server_t* server, event_t* event)
{
	server_ep_del_event(server, event);

	if (event->close)
		event->close(server, event);

	if (event->prev)
		event->prev->next = event->next;
	if (event->next)
		event->next->prev = event->prev;

	if (server->events.head == event)
		server->events.head = event->next;
	if (server->events.tail == event)
	{
		if (event->next)
			server->events.tail = event->next;
		else if (event->prev)
			server->events.tail = event->prev;
		else
			server->events.tail = server->events.head;
	}

	free(event);
}

void 
server_handle_event(server_t* server, event_t* event, u32 events)
{
	if (events & (EPOLLERR | EPOLLHUP))
	{
		server_close_event(server, event);
	}
	else if (events & EPOLLIN)
	{
		event->read(server, event);
	}
}

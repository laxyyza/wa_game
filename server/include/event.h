#ifndef _SERVER_EVENT_H_
#define _SERVER_EVENT_H_

#include "int.h"

typedef struct server server_t;
typedef struct event event_t;

typedef void (*event_read_t)(server_t* server, event_t* event);
typedef void (*event_close_t)(server_t* server, event_t* event);

typedef struct event
{
	i32 fd;
	void* data;
	event_read_t read;
	event_close_t close;

	struct event* next;
	struct event* prev;
} event_t;

void server_add_event(server_t* server, i32 fd, void* data, event_read_t read, event_close_t close);
void server_close_event(server_t* server, event_t* event);
void server_close_all_events(server_t* server);
void server_handle_event(server_t* server, event_t* event, u32 events);

#endif // _SERVER_EVENT_H_

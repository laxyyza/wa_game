#ifndef _WAAPP_STATE_H_
#define _WAAPP_STATE_H_

#include <stdbool.h>
#include "wa.h"

typedef struct waapp waapp_t;

typedef void* (*state_init_callback_t)(waapp_t* app);
typedef void (*state_callback_t)(waapp_t* app, void* data);
typedef i32  (*state_event_callback_t)(waapp_t* app, const wa_event_t* ev);

#define STATE_DO_CLEANUP 0x01
#define STATE_CLEANED_UP 0x02

typedef struct 
{
	void*					data;
	u8						flags;
	state_init_callback_t	init;
	state_callback_t		enter;
	state_callback_t		update;
	state_event_callback_t	event;
	state_callback_t		exit;
	state_callback_t		cleanup;
} waapp_state_t;

typedef struct 
{
	waapp_state_t* current;
	waapp_state_t* prev;
	bool cleanup_pending;

	struct {
		waapp_state_t main_menu;
		waapp_state_t game;
		waapp_state_t map_editor;
	} states;
} waapp_state_manager_t;

void waapp_state_manager_init(waapp_t* app);
void waapp_state_switch(waapp_t* app, waapp_state_t* state);
void waapp_state_update(wa_window_t* window, waapp_t* app);

#endif // _WAAPP_STATE_H_

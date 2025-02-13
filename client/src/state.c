#include "state.h"
#include "app.h"
#include "renderer.h"
#include "main_menu.h"
#include "gui/gui.h"
#include "map_editor.h"
#include "nuklear.h"

void
waapp_state_manager_init(waapp_t* app)
{
	waapp_state_manager_t* sm = &app->sm;

	sm->states.main_menu.init = main_menu_init;
	sm->states.main_menu.enter = (state_callback_t)main_menu_enter;
	sm->states.main_menu.update = (state_callback_t)main_menu_update;
	sm->states.main_menu.event = (state_event_callback_t)main_menu_event;
	sm->states.main_menu.exit = (state_callback_t)main_menu_exit;
	sm->states.main_menu.cleanup = (state_callback_t)main_menu_cleanup;
	sm->states.main_menu.flags = STATE_CLEANED_UP;

	sm->states.game.init = game_init;
	sm->states.game.update = (state_callback_t)game_update;
	sm->states.game.event = (state_event_callback_t)game_event;
	sm->states.game.cleanup = (state_callback_t)game_cleanup;
	sm->states.game.flags = STATE_DO_CLEANUP | STATE_CLEANED_UP;

	sm->states.map_editor.init = map_editor_init;
	sm->states.map_editor.enter = (state_callback_t)map_editor_enter;
	sm->states.map_editor.update = (state_callback_t)map_editor_update;
	sm->states.map_editor.event = (state_event_callback_t)map_editor_event;
	sm->states.map_editor.exit = (state_callback_t)map_editor_exit;
	sm->states.map_editor.cleanup = (state_callback_t)map_editor_cleanup;
	sm->states.map_editor.flags = STATE_CLEANED_UP;

	waapp_state_switch(app, &sm->states.main_menu);
}

void 
waapp_state_switch(waapp_t* app, waapp_state_t* state)
{
	waapp_state_manager_t* sm = &app->sm;

	if (sm->current == state)
		return;

	sm->prev = sm->current;
	if (sm->prev)
	{
		if (sm->prev->exit)
			sm->prev->exit(app, sm->prev->data);

		if (sm->prev->flags & STATE_DO_CLEANUP)
			sm->cleanup_pending = true;
	}

	if (state->flags & STATE_CLEANED_UP && state->init)
	{
		state->data = state->init(app);
		state->flags ^= STATE_CLEANED_UP;
	}
	if (state->enter)
		state->enter(app, state->data);

	sm->current = state;
}

static void
waapp_state_cleanup(waapp_t* app, waapp_state_t* state)
{
	if (state->cleanup)
		state->cleanup(app, state->data);
	state->data = NULL;
	state->flags |= STATE_CLEANED_UP;
}

void
waapp_state_update(wa_window_t* window, waapp_t* app)
{
	waapp_state_manager_t* sm = &app->sm;
	wa_state_t* state = wa_window_get_state(window);

	ren_clear(&app->ren, &app->bg_color);

	sm->current->update(app, sm->current->data);
	if (app->headless == false)
	{
		gui_new_frame(app);
		app->on_ui = nk_window_is_any_hovered(app->nk_ctx);
		gui_render(app);
		if (state->window.vsync == false)
			wa_swap_buffers(window);
	}

	if (sm->cleanup_pending)
	{
		waapp_state_cleanup(app, sm->prev);
		sm->cleanup_pending = false;
	}
}

void
waapp_state_manager_cleanup(waapp_t* app)
{
	waapp_state_manager_t* sm = &app->sm;

	if ((sm->states.main_menu.flags & STATE_CLEANED_UP) == 0)
		waapp_state_cleanup(app, &sm->states.main_menu);
	if ((sm->states.map_editor.flags & STATE_CLEANED_UP) == 0)
	{
		waapp_state_cleanup(app, &sm->states.map_editor);
	}
	if ((sm->states.game.flags & STATE_CLEANED_UP) == 0)
		waapp_state_cleanup(app, &sm->states.game);
}

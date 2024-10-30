#include "state.h"
#include "app.h"
#include "renderer.h"
#include "main_menu.h"
#include "gui/gui.h"

void
waapp_state_manager_init(waapp_t* app)
{
	waapp_state_manager_t* sm = &app->sm;

	sm->states.main_menu.init = main_menu_init;
	sm->states.main_menu.update = main_menu_update;
	sm->states.main_menu.event = main_menu_event;
	sm->states.main_menu.exit = main_menu_exit;

	sm->states.game.init = game_init;
	sm->states.game.update = game_update;
	sm->states.game.event = game_event;
	sm->states.game.exit = game_exit;

	waapp_state_switch(app, &sm->states.main_menu);
}

void 
waapp_state_switch(waapp_t* app, waapp_state_t* state)
{
	waapp_state_manager_t* sm = &app->sm;

	sm->prev = sm->current;
	if (sm->prev && sm->prev->exit)
		sm->switch_appending = true;

	if (state->init)
		state->init(app, state->data);

	sm->current = state;
}

void
waapp_state_update(wa_window_t* window, waapp_t* app)
{
	waapp_state_manager_t* sm = &app->sm;
	wa_state_t* state = wa_window_get_state(window);

	gui_new_frame(app);
	ren_clear(&app->ren, &app->bg_color);

	sm->current->update(app, sm->current->data);

	if (sm->switch_appending)
	{
		sm->prev->exit(app, sm->prev->data);
		sm->switch_appending = false;
	}

	gui_render(app);

	if (state->window.vsync == false)
		wa_swap_buffers(window);
}

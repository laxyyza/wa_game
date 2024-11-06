#define _GNU_SOURCE
#include "app.h"
#include "opengl.h"
#include <wa_keys.h>
#include <wa_cursor.h>
#include <string.h>
#include "gui/gui.h"
#include <time.h>
#include "nuklear.h"
#include "cutils.h"

static void 
nk_handle_input(waapp_t* app, const wa_event_key_t* ev)
{
	struct nk_context* ctx = app->nk_ctx;
	const bool pressed = ev->pressed;
	wa_state_t* state = wa_window_get_state(app->window);
	const u8* keymap = state->key_map;

	nk_input_begin(ctx);
	if (ev->ascii >= 32 && ev->ascii <= 126 && ev->pressed)
	{
		nk_input_char(ctx, ev->ascii);
	}
	else
	{
		switch (ev->key)
		{
			case WA_KEY_RSHIFT:
			case WA_KEY_LSHIFT:	nk_input_key(ctx, NK_KEY_SHIFT, pressed); break;
			case WA_KEY_ENTER:	nk_input_key(ctx, NK_KEY_ENTER, pressed); break;
			case WA_KEY_TAB:	nk_input_key(ctx, NK_KEY_TAB,	pressed); break;
			case WA_KEY_DEL:	nk_input_key(ctx, NK_KEY_DEL,	pressed); break;
			case WA_KEY_LCTRL:	nk_input_key(ctx, NK_KEY_CTRL,	pressed); break;
			case WA_KEY_UP:		nk_input_key(ctx, NK_KEY_UP,	pressed); break;
			case WA_KEY_DOWN:	nk_input_key(ctx, NK_KEY_DOWN,	pressed); break;
			case WA_KEY_LEFT: 
				if (keymap[WA_KEY_LCTRL])
					nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, pressed);
				else
					nk_input_key(ctx, NK_KEY_LEFT, pressed);
				break;
			case WA_KEY_RIGHT: 
				if (keymap[WA_KEY_LCTRL])
					nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, pressed);
				else
					nk_input_key(ctx, NK_KEY_RIGHT, pressed);
				break;
			case WA_KEY_BACKSPACE:	nk_input_key(ctx, NK_KEY_BACKSPACE,	pressed); break;
			case WA_KEY_HOME:		nk_input_key(ctx, NK_KEY_TEXT_START,pressed); break;
			case WA_KEY_END:		nk_input_key(ctx, NK_KEY_TEXT_END,	pressed); break;
			case WA_KEY_Z:			nk_input_key(ctx, NK_KEY_TEXT_UNDO, pressed && keymap[WA_KEY_LCTRL]); break;
			case WA_KEY_R:			nk_input_key(ctx, NK_KEY_TEXT_REDO, pressed && keymap[WA_KEY_LCTRL]); break;
			case WA_KEY_C:			nk_input_key(ctx, NK_KEY_COPY,		pressed && keymap[WA_KEY_LCTRL]); break;
			case WA_KEY_X:			nk_input_key(ctx, NK_KEY_CUT,		pressed && keymap[WA_KEY_LCTRL]); break;
			case WA_KEY_V:			nk_input_key(ctx, NK_KEY_PASTE,		pressed && keymap[WA_KEY_LCTRL]); break;
			case WA_KEY_A:			nk_input_key(ctx, NK_KEY_TEXT_SELECT_ALL, pressed && keymap[WA_KEY_LCTRL]); break;
			default:
				break;
		}
	}
	nk_input_end(ctx);
}

static void
waapp_handle_key(waapp_t* app, wa_window_t* window, const wa_event_key_t* ev)
{
    wa_state_t* state = wa_window_get_state(window);

	nk_handle_input(app, ev);

	switch (ev->key)
	{
		case WA_KEY_F:
			if (ev->pressed && app->on_ui == false)
                wa_window_set_fullscreen(window, !(state->window.state & WA_STATE_FULLSCREEN));
			break;
		default:
			break;
	}
}

static void 
waapp_handle_pointer(waapp_t* app, const wa_event_pointer_t* ev)
{
	app->mouse.x = ev->x;
	app->mouse.y = ev->y;
}

static void
waapp_handle_mouse_wheel(waapp_t* app, const wa_event_wheel_t* ev)
{
	f32 dir = (ev->value > 0) ? -1.0 : 1.0;

	if (app->on_ui)
		gui_scroll(app, 0.0, dir);
}

static void
waapp_resize(waapp_t* app, const wa_event_resize_t* ev)
{
	ren_viewport(&app->ren, ev->w, ev->h);
	if (app->game && app->game->lock_cam)
		game_lock_cam(app->game);
}

static void 
waapp_handle_mouse(waapp_t* app, wa_window_t* window, const wa_event_mouse_t* ev)
{
	if (ev->button == app->keybind.cam_move)
	{
		if (ev->pressed)
			wa_set_cursor_shape(window, WA_CURSOR_ALL_SCROLL);
		else
			wa_set_cursor_shape(window, WA_CURSOR_DEFAULT);
	}
}

static void
waapp_event(wa_window_t* window, const wa_event_t* ev, void* data)
{
    waapp_t* app = data;

	switch (ev->type)
	{
		case WA_EVENT_KEYBOARD:
			waapp_handle_key(app, window, &ev->keyboard);
			break;
		case WA_EVENT_POINTER:
			waapp_handle_pointer(app, &ev->pointer);
			break;
		case WA_EVENT_RESIZE:
			waapp_resize(app, &ev->resize);
			break;
		case WA_EVENT_MOUSE_BUTTON:
			waapp_handle_mouse(app, window, &ev->mouse);
			break;
		case WA_EVENT_MOUSE_WHEEL:
			waapp_handle_mouse_wheel(app, &ev->wheel);
			break;
		default:
			break;
	}

	if (app->sm.current->event) 
	{
		app->sm.current->event(app, app->sm.current->data, ev);
	}
}

static void 
waapp_close(_WA_UNUSED wa_window_t* window, _WA_UNUSED void* data)
{

}


i32 
waapp_init(waapp_t* app, i32 argc, const char** argv)
{
    const char* title = "WA OpenGL";
    const char* app_id = "wa_opengl";
    i32 w = 1280;
    i32 h = 720;
    bool fullscreen = false;
    wa_state_t* state;

	nlog_set_name("");

    if (argc >= 2 && strcmp(argv[1], "fullscreen") == 0)
        fullscreen = true;

    if ((app->window = wa_window_create(title, w, h, fullscreen)) == NULL)
	{
        return -1;
	}

    state = wa_window_get_state(app->window);
    state->window.wayland.app_id = app_id;
    state->user_data = app;
    state->callbacks.update = (void (*)(wa_window_t* window, void* data)) waapp_state_update;
    state->callbacks.event = waapp_event;
    state->callbacks.close = waapp_close;

    app->bg_color = rgba(0x000000FF);

    if (waapp_opengl_init(app) == false)
    {
        wa_window_delete(app->window);
        return -1;
    }
	mmframes_init(&app->mmf);

	client_net_init(app);

	waapp_state_manager_init(app);	

	app->keybind.cam_move = WA_MOUSE_RIGHT;

	app->grass_tex = texture_load("res/grass.png", TEXTURE_NEAREST);
	app->grass_tex->name = "Grass";
	app->block_tex = texture_load("res/block.png", TEXTURE_NEAREST);
	app->block_tex->name = "Block";

	app->min_zoom = 0.4;
	app->max_zoom = 4.0;
	app->clamp_cam = true;

    return 0;
}

void 
waapp_set_max_fps(waapp_t* app, f64 max_fps)
{
	app->fps_interval = (1.0 / max_fps) * 1e9;
	app->max_fps = max_fps;
}

void 
waapp_run(waapp_t* app)
{
	wa_state_t* state = wa_window_get_state(app->window);

	clock_gettime(CLOCK_MONOTONIC, &app->last_time);
	app->start_time = app->end_time = app->last_time;

	wa_window_vsync(app->window, true);
	waapp_set_max_fps(app, 144.0);

	while (wa_window_running(app->window))
	{
		client_net_poll(app, &app->start_time, &app->end_time);

		if (state->window.vsync == false)
			waapp_state_update(app->window, app);
	}
}

void 
waapp_cleanup(waapp_t* app)
{
	mmframes_free(&app->mmf);
    waapp_opengl_cleanup(app);
    wa_window_delete(app->window);
}

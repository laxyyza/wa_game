#define _GNU_SOURCE
#include "app.h"
#include "opengl.h"
#include <wa_keys.h>
#include <wa_cursor.h>
#include <string.h>
#include "gui/gui.h"
#include "nuklear.h"
#include "cutils.h"
#include <getopt.h>

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

static void
waapp_print_help(const char* path)
{
	printf(
		"Usage:\n\t%s [options]\n\n"\
		"	-d, --disable-debug\tDisable OpenGL Debug.\n"\
		"	--ssp-debug\t\tPrint IN SSP Packets.\n"\
		"	-f, --fullscreen\tOpen in fullscreen.\n"\
		"	-h, --help\tShow this message.\n\n",
		path
	);
}

static i32 
waapp_argv(waapp_t* app, i32 argc, char* const* argv, bool* fullscreen)
{
	i32 opt;

	struct option long_options[] = {
		{"disable-debug",	no_argument, 0, 'd'},
		{"ssp-debug",		no_argument, 0, 's'},
		{"fullscreen",		no_argument, 0, 'f'},
		{"help",			no_argument, 0, 'h'},
		{0, 0, 0, 0}
	};

	while ((opt = getopt_long(argc, argv, "dfh", long_options, NULL)) != -1)
	{
		switch (opt)
		{
			case 'd':
				app->disable_debug = true;
				break;
			case 'f':
				*fullscreen = true;
				break;
			case 'h':
				waapp_print_help(argv[0]);
				return -1;
			case 's':
				app->net.def.ssp_ctx.debug = true;
				break;
			default:
				return -1;
		}
	}

	return 0;
}

i32 
waapp_init(waapp_t* app, i32 argc, char* const* argv)
{
#ifdef __linux__
    const char* title = "WA Game linux v" VERSION " (" BUILD_TYPE ")";
#elif _WIN32
    const char* title = "WA Game win32 v" VERSION " (" BUILD_TYPE ")";
#else
    const char* title = "WA Game unknown v" VERSION " (" BUILD_TYPE ")";
#endif
    const char* app_id = "wa_game";
    i32 w = 1280;
    i32 h = 720;
    bool fullscreen = false;
    wa_state_t* state;

	if (waapp_argv(app, argc, argv, &fullscreen) == -1)
		return -1;

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

	nano_timer_init(&app->timer);

    return 0;
}

void 
waapp_set_max_fps(waapp_t* app, f64 max_fps)
{
	app->fps_interval = (1.0 / max_fps) * 1e9;
	app->fps_interval_ms = (1.0 / max_fps) * 1e3;
	app->max_fps = max_fps;
}

void 
waapp_run(waapp_t* app)
{
	wa_state_t* state = wa_window_get_state(app->window);

	wa_window_vsync(app->window, true);
	waapp_set_max_fps(app, 144.0);

	while (wa_window_running(app->window))
	{
		client_net_poll(app);

		if (state->window.vsync == false)
			waapp_state_update(app->window, app);
	}
}

void 
waapp_cleanup(waapp_t* app)
{
	texture_del(app->grass_tex);
	texture_del(app->block_tex);

	waapp_state_manager_cleanup(app);
	mmframes_free(&app->mmf);
	gui_free(app);
    waapp_opengl_cleanup(app);
	client_net_cleanup(app);
    wa_window_delete(app->window);
}

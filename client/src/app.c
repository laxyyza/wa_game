#define _GNU_SOURCE
#include "app.h"
#include "opengl.h"
#include <wa_keys.h>
#include <wa_cursor.h>
#include <string.h>
#include "gui/gui.h"
#include <time.h>
#include "nuklear.h"

static void 
client_shoot(waapp_t* app)
{
	const player_t* player = app->player;

	const vec2f_t mpos = screen_to_world(app, &app->mouse);
	vec2f_t dir = vec2f(
		mpos.x - (player->core->pos.x + (player->core->size.x / 2)),
		mpos.y - (player->core->pos.y + (player->core->size.y / 2))
	);
	vec2f_norm(&dir);

	cg_projectile_t* proj = coregame_player_shoot(&app->game, app->player->core, dir);

	net_udp_player_shoot_t* shoot = mmframes_alloc(&app->mmf, sizeof(net_udp_player_shoot_t));
	shoot->shoot_dir = dir;
	shoot->shoot_pos = proj->rect.pos;
	ssp_segbuff_add(&app->net.udp.buf, NET_UDP_PLAYER_SHOOT, sizeof(net_udp_player_shoot_t), shoot);
}

void
waapp_lock_cam(waapp_t* app)
{
	player_t* player = app->player;
	vec2f_t origin = vec2f(
		player->core->pos.x + (player->core->size.x / 2),
		player->core->pos.y + (player->core->size.y / 2)
	);

	const vec2f_t* viewport = &app->ren.viewport;
	app->cam.x = -(origin.x * app->ren.scale.x - (viewport->x / 2));
	app->cam.y = -(origin.y * app->ren.scale.y - (viewport->y / 2));
	ren_set_view(&app->ren, &app->cam);

	player->core->cursor = screen_to_world(app, &app->mouse);
	ssp_segbuff_add(&app->net.udp.buf, NET_UDP_PLAYER_CURSOR, sizeof(vec2f_t), &player->core->cursor);
}

static void
game_draw(waapp_t* app)
{
    waapp_opengl_draw(app);
	app->frames++;
}

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
			if (ev->pressed)
                wa_window_set_fullscreen(window, !(state->window.state & WA_STATE_FULLSCREEN));
			break;
		default:
			break;
	}
}

static void
game_handle_key(waapp_t* app, wa_window_t* window, const wa_event_key_t* ev)
{
    wa_state_t* state = wa_window_get_state(window);
	player_t* player = app->player;

	switch (ev->key)
	{
		case WA_KEY_V:
			if (ev->pressed)
                wa_window_vsync(window, !state->window.vsync);
			break;
		case WA_KEY_W:
			if (ev->pressed)
				player->movement_dir |= PLAYER_DIR_UP;
			else
				player->movement_dir ^= PLAYER_DIR_UP;
			break;
		case WA_KEY_S:
			if (ev->pressed)
				player->movement_dir |= PLAYER_DIR_DOWN;
			else
				player->movement_dir ^= PLAYER_DIR_DOWN;
			break;
		case WA_KEY_A:
			if (ev->pressed)
				player->movement_dir |= PLAYER_DIR_LEFT;
			else
				player->movement_dir ^= PLAYER_DIR_LEFT;
			break;
		case WA_KEY_D:
			if (ev->pressed)
				player->movement_dir |= PLAYER_DIR_RIGHT;
			else
				player->movement_dir ^= PLAYER_DIR_RIGHT;
			break;
		case WA_KEY_SPACE:
			if (ev->pressed && ((app->lock_cam = !app->lock_cam)))
				waapp_lock_cam(app);
			break;
		case WA_KEY_T:
			if (ev->pressed)
				app->trigger_shooting = !app->trigger_shooting;
			break;
		default:
			break;
	}
}

static void 
game_handle_pointer(waapp_t* app, UNUSED const wa_event_pointer_t* ev)
{
	if (app->player)
	{
		app->player->core->cursor = screen_to_world(app, &app->mouse);
		ssp_segbuff_add(&app->net.udp.buf, NET_UDP_PLAYER_CURSOR, sizeof(vec2f_t), &app->player->core->cursor);
	}
}

static void 
waapp_handle_pointer(waapp_t* app, const wa_event_pointer_t* ev)
{
	app->mouse.x = ev->x;
	app->mouse.y = ev->y;

	if (app->player)
	{
		app->player->core->cursor = screen_to_world(app, &app->mouse);
		ssp_segbuff_add(&app->net.udp.buf, NET_UDP_PLAYER_CURSOR, sizeof(vec2f_t), &app->player->core->cursor);
	}
}

static void 
game_handle_mouse_button(waapp_t* app, const wa_event_mouse_t* ev)
{
	if (ev->button == WA_MOUSE_LEFT && ev->pressed)
	{
		client_shoot(app);
	}
}

void 
game_handle_mouse_wheel(waapp_t* app, const wa_event_wheel_t* ev)
{
	f32 dir = (ev->value > 0) ? -1.0 : 1.0;

	if (app->on_ui)
		return;

	vec2f_t old_wpos = screen_to_world(app, &app->mouse);

	if (dir > 0)
	{
		app->ren.scale.x += 0.1;
		app->ren.scale.y += 0.1;
	}
	else
	{
		app->ren.scale.x -= 0.1;
		app->ren.scale.y -= 0.1;

		if (app->ren.scale.x < 0.1)
			app->ren.scale.x = 0.1;
		if (app->ren.scale.y < 0.1)
			app->ren.scale.y = 0.1;
	}
	ren_set_scale(&app->ren, &app->ren.scale);

	if (app->lock_cam)
		waapp_lock_cam(app);
	else
	{
		vec2f_t new_wpos = screen_to_world(app, &app->mouse);

		app->cam.x = app->cam.x - (old_wpos.x - new_wpos.x) * app->ren.scale.x;
		app->cam.y = app->cam.y - (old_wpos.y - new_wpos.y) * app->ren.scale.y;

		ren_set_view(&app->ren, &app->cam);
	}
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
	if (app->lock_cam)
		waapp_lock_cam(app);
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
		app->sm.current->event(app, ev);
	}
}

static void 
waapp_close(_WA_UNUSED wa_window_t* window, _WA_UNUSED void* data)
{

}

static void 
on_player_free(cg_player_t* player, void* data)
{
	waapp_t* app = data;
	ght_del(&app->players, player->id);
}

static void 
update_logic(waapp_t* app)
{
	player_t* player = app->player;

	if (player)
	{
		coregame_set_player_dir(player->core, player->movement_dir);

		coregame_update(&app->game);

		if (app->prev_pos.x != player->core->pos.x || app->prev_pos.y != player->core->pos.y)
		{
			if (app->lock_cam)
				waapp_lock_cam(app);
			app->prev_pos = player->core->pos;
		}
		if (app->prev_dir.x != player->core->dir.x || app->prev_dir.y != player->core->dir.y)
		{
			ssp_segbuff_add(&app->net.udp.buf, NET_UDP_PLAYER_DIR, sizeof(net_udp_player_dir_t), &player->core->dir);
			app->prev_dir = player->core->dir;
		}

		if (app->trigger_shooting)
			client_shoot(app);

		client_net_try_udp_flush(app);
	}
}

void 
game_init(waapp_t* app, UNUSED void* data)
{
	coregame_init(&app->game, true);
	app->game.user_data = app;
	app->game.player_free_callback = on_player_free;

	app->tank_bottom_tex = texture_load("res/tank_bottom.png", TEXTURE_NEAREST);
	app->tank_top_tex = texture_load("res/tank_top.png", TEXTURE_NEAREST);
	app->lock_cam = true;

	rect_init(&app->world_border, app->game.world_border.pos, app->game.world_border.size, 0xFF0000FF, NULL);
	ght_init(&app->players, 10, free);

	app->keybind.cam_move = WA_MOUSE_RIGHT;
}

void 
game_update(waapp_t* app, UNUSED void* data)
{
	clock_gettime(CLOCK_MONOTONIC, &app->start_time);

	update_logic(app);
	game_draw(app);

	if (app->update_vync)
	{
		wa_window_vsync(app->window, app->tmp_vsync);
		app->update_vync = false;
	}

	clock_gettime(CLOCK_MONOTONIC, &app->end_time);

	f64 elapsed_time_sec = (app->end_time.tv_sec - app->last_time.tv_sec) + 
							(app->end_time.tv_nsec - app->last_time.tv_nsec) / 1e9;
	if (elapsed_time_sec > 1.0)
	{
		app->fps = app->frames;
		app->frames = 0;
		app->last_time = app->end_time;
	}
}

i32
game_event(waapp_t* app, const wa_event_t* ev)
{
	wa_window_t* window = app->window;

	switch (ev->type)
	{
		case WA_EVENT_KEYBOARD:
			game_handle_key(app, window, &ev->keyboard);
			return 0;
		case WA_EVENT_POINTER:
			game_handle_pointer(app, &ev->pointer);
			return 0;
		case WA_EVENT_MOUSE_BUTTON:
			game_handle_mouse_button(app, &ev->mouse);
			return 0;
		case WA_EVENT_MOUSE_WHEEL:
			game_handle_mouse_wheel(app, &ev->wheel);
			return 0;
		default:
			return 1;
	}
}

void 
game_exit(waapp_t* app, UNUSED void* data)
{
	ren_delete_bro(app->line_bro);
	ght_destroy(&app->players);
	coregame_cleanup(&app->game);
}

void
waapp_move_cam(waapp_t* app)
{
    wa_state_t* state = wa_window_get_state(app->window);
    vec3f_t* cam = &app->cam;

    if (app->mouse.x != app->mouse_prev.x || app->mouse.y != app->mouse_prev.y)
    {
        if (state->mouse_map[app->keybind.cam_move])
        {
			app->lock_cam = false;
            vec2f_t diff = vec2f(
                app->mouse_prev.x - app->mouse.x,
                app->mouse_prev.y - app->mouse.y
            );
            cam->x -= diff.x;
            cam->y -= diff.y;
            ren_set_view(&app->ren, cam);
        }

        app->mouse_prev = app->mouse;
    }
}

i32 
waapp_init(waapp_t* app, i32 argc, const char** argv)
{
    const char* title = "WA OpenGL";
    const char* app_id = "wa_opengl";
    i32 w = 640;
    i32 h = 480;
    bool fullscreen = false;
    wa_state_t* state;

    if (argc >= 2 && strcmp(argv[1], "fullscreen") == 0)
        fullscreen = true;

    if ((app->window = wa_window_create(title, w, h, fullscreen)) == NULL)
        return -1;

    state = wa_window_get_state(app->window);
    state->window.wayland.app_id = app_id;
    state->user_data = app;
    state->callbacks.update = (void (*)(wa_window_t* window, void* data)) waapp_state_update;
    state->callbacks.event = waapp_event;
    state->callbacks.close = waapp_close;

    app->bg_color = rgba(0x333333FF);

    if (waapp_opengl_init(app) == false)
    {
        wa_window_delete(app->window);
        return -1;
    }
	app->line_bro = ren_new_bro(DRAW_LINES, 1024, NULL, NULL, &app->ren.default_bro->shader);
	mmframes_init(&app->mmf);

	client_net_init(app);

	waapp_state_manager_init(app);	

	app->keybind.cam_move = WA_MOUSE_RIGHT;

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
	coregame_cleanup(&app->game);
	ren_delete_bro(app->line_bro);
    waapp_opengl_cleanup(app);
    wa_window_delete(app->window);
}

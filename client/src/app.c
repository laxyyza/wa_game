#define _GNU_SOURCE
#include "app.h"
#include "opengl.h"
#include <wa_keys.h>
#include <wa_cursor.h>
#include <string.h>
#include "gui/gui.h"
#include <time.h>

static void 
client_shoot(waapp_t* app)
{
	const player_t* player = app->player;
	const vec2f_t mpos = vec2f(
		(app->mouse.x - app->cam.x),
		(app->mouse.y - app->cam.y)
	);
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

static void
waapp_draw(_WA_UNUSED wa_window_t* window, void* data)
{
	waapp_t* app = data;
	player_t* player = app->player;
	
	client_net_poll(app, 0);

	if (player)
	{
		coregame_set_player_dir(player->core, player->movement_dir);

		coregame_update(&app->game);

		if (app->prev_pos.x != player->core->pos.x || app->prev_pos.y != player->core->pos.y)
		{
			if (app->lock_cam)
			{
				const vec2f_t* viewport = &app->ren.viewport;
				app->cam.x = -(player->core->pos.x - (viewport->x / 2) + (player->core->size.x / 2));
				app->cam.y = -(player->core->pos.y - (viewport->y / 2) + (player->core->size.y / 2));
				ren_set_view(&app->ren, &app->cam);

				player->core->cursor = vec2f(
					app->mouse.x - app->cam.x,
					app->mouse.y - app->cam.y
				);
				ssp_segbuff_add(&app->net.udp.buf, NET_UDP_PLAYER_CURSOR, sizeof(vec2f_t), &player->core->cursor);
			}
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
    waapp_opengl_draw(app);
}

static void
waapp_event(wa_window_t* window, const wa_event_t* ev, void* data)
{
    waapp_t* app = data;
    wa_state_t* state = wa_window_get_state(window);
	player_t* player = app->player;

    if (ev->type == WA_EVENT_KEYBOARD)
    {
        if (ev->keyboard.pressed)
        {
            if (ev->keyboard.key == WA_KEY_F)
            {
                wa_window_set_fullscreen(window, !(state->window.state & WA_STATE_FULLSCREEN));
            }
            else if (ev->keyboard.key == WA_KEY_V)
            {
                wa_window_vsync(window, !state->window.vsync);
            }
			else if (ev->keyboard.key == WA_KEY_W)
			{
				player->movement_dir |= PLAYER_DIR_UP;
			}
			else if (ev->keyboard.key == WA_KEY_S)
			{
				player->movement_dir |= PLAYER_DIR_DOWN;
			}
			else if (ev->keyboard.key == WA_KEY_A)
			{
				player->movement_dir |= PLAYER_DIR_LEFT;
			}
			else if (ev->keyboard.key == WA_KEY_D)
			{
				player->movement_dir |= PLAYER_DIR_RIGHT;
			}
			else if (ev->keyboard.key == WA_KEY_SPACE)
			{
				app->lock_cam = !app->lock_cam;
			}
			else if (ev->keyboard.key == WA_KEY_T)
			{
				app->trigger_shooting = !app->trigger_shooting;
			}
        }
		else
		{
			if (ev->keyboard.key == WA_KEY_W)
			{
				player->movement_dir ^= PLAYER_DIR_UP;
			}
			else if (ev->keyboard.key == WA_KEY_S)
			{
				player->movement_dir ^= PLAYER_DIR_DOWN;
			}
			else if (ev->keyboard.key == WA_KEY_A)
			{
				player->movement_dir ^= PLAYER_DIR_LEFT;
			}
			else if (ev->keyboard.key == WA_KEY_D)
			{
				player->movement_dir ^= PLAYER_DIR_RIGHT;
			}
		}
    }
    else if (ev->type == WA_EVENT_RESIZE)
    {
        ren_viewport(&app->ren, ev->resize.w, ev->resize.h);
        // shader_bind(&app->shader);
        // shader_uniform_mat4f(&app->shader, "mvp", &app->ren.proj);
    }
    else if (ev->type == WA_EVENT_POINTER)
    {
        app->mouse.x = ev->pointer.x;
        app->mouse.y = ev->pointer.y;

		if (app->player)
		{
			app->player->core->cursor = vec2f(
				app->mouse.x - app->cam.x,
				app->mouse.y - app->cam.y
			);
			ssp_segbuff_add(&app->net.udp.buf, NET_UDP_PLAYER_CURSOR, sizeof(vec2f_t), &app->player->core->cursor);
		}
    }
    else if (ev->type == WA_EVENT_MOUSE_BUTTON)
    {
        if (ev->mouse.button == WA_MOUSE_RIGHT)
        {
            if (ev->mouse.pressed)
            {
                wa_set_cursor_shape(window, WA_CURSOR_ALL_SCROLL);
                app->mouse_prev = app->mouse;
            }
            else
                wa_set_cursor_shape(window, WA_CURSOR_DEFAULT);
        }
		else if (ev->mouse.button == WA_MOUSE_LEFT && ev->mouse.pressed)
		{
			client_shoot(app);
		}
    }
    else if (ev->type == WA_EVENT_MOUSE_WHEEL)
    {
        f32 dir = (ev->wheel.value > 0) ? -1.0 : 1.0;

        gui_scroll(app, 0.0, dir);

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
    state->callbacks.draw = waapp_draw;
    state->callbacks.event = waapp_event;
    state->callbacks.close = waapp_close;

    app->bg_color = rgba(0x333333FF);
	app->lock_cam = true;

    // if (argc == 2 && fullscreen == false)
    //     app->texture_path = argv[1];
    // else if (argc == 3)
    //     app->texture_path = argv[2];
    // else 
    //     app->texture_path = "res/test.png";
    //
    if (waapp_opengl_init(app) == false)
    {
        wa_window_delete(app->window);
        return -1;
    }

	coregame_init(&app->game, true);
	app->game.user_data = app;
	app->game.player_free_callback = on_player_free;

	app->tank_bottom_tex = texture_load("res/tank_bottom.png", TEXTURE_NEAREST);
	app->tank_top_tex = texture_load("res/tank_top.png", TEXTURE_NEAREST);

	rect_init(&app->world_border, app->game.world_border.pos, app->game.world_border.size, 0xFF0000FF, NULL);
	ght_init(&app->players, 10, free);

	app->line_bro = ren_new_bro(DRAW_LINES, 4, NULL, NULL, &app->ren.default_bro->shader);

	const char* ipaddr = (argc == 1) ? "127.0.0.1" : argv[1];

	client_net_init(app, ipaddr, 8080);

	mmframes_init(&app->mmf);

    return 0;
}

void 
waapp_run(waapp_t* app)
{
    wa_window_mainloop(app->window);
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

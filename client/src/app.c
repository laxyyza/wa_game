#include "app.h"
#include "opengl.h"
#include <wa_keys.h>
#include <wa_cursor.h>
#include <string.h>
#include "gui/gui.h"

static void
waapp_draw(_WA_UNUSED wa_window_t* window, void* data)
{

	waapp_t* app = data;
	player_t* player = app->player;
	
	client_net_poll(app, 0);

	coregame_set_player_dir(player->core, player->movement_dir);

	if (player->movement_dir)
		player->rect.rotation = atan2(player->core->dir.y, player->core->dir.x) + M_PI / 2;

	coregame_update(&app->game);

    const vec2f_t mpos = vec2f(
        (app->mouse.x - app->cam.x),
        (app->mouse.y - app->cam.y)
    );
	const vec2f_t origin = rect_origin(&player->rect);

	player->top.rotation = angle(&origin, 
								   &mpos);

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
			const player_t* player = app->player;
			const vec2f_t mpos = vec2f(
				(app->mouse.x - app->cam.x),
				(app->mouse.y - app->cam.y)
			);
			vec2f_t dir = vec2f(
				mpos.x - (player->core->rect.pos.x + (player->core->rect.size.x / 2)),
				mpos.y - (player->core->rect.pos.y + (player->core->rect.size.y / 2))
			);
			vec2f_norm(&dir);

			cg_projectile_t* proj = coregame_player_shoot(&app->game, app->player->core, dir);
			projectile_new(app, proj);
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
on_projectile_free(cg_projectile_t* cg_proj, void* data)
{
	waapp_t* app = data;
	ght_del(&app->projectiles, cg_proj->id);
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

	coregame_init(&app->game);
	app->game.user_data = app;
	app->game.proj_free_callback = on_projectile_free;
	app->game.player_free_callback = on_player_free;

	app->tank_bottom_tex = texture_load("res/tank_bottom.png", TEXTURE_NEAREST);
	app->tank_top_tex = texture_load("res/tank_top.png", TEXTURE_NEAREST);

	rect_init(&app->world_border, app->game.world_border.pos, app->game.world_border.size, 0xFF0000FF, NULL);
	ght_init(&app->projectiles, 10, free);
	ght_init(&app->players, 10, free);

	app->player = player_new(app, "test");

	app->line_bro = ren_new_bro(DRAW_LINES, 4, NULL, NULL, &app->ren.default_bro->shader);

	client_net_init(app, "127.0.0.1", 8080);

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
	coregame_cleanup(&app->game);
	ren_delete_bro(app->line_bro);
    waapp_opengl_cleanup(app);
    wa_window_delete(app->window);
}

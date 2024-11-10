#include "game.h"
#include "game_draw.h"
#include "app.h"
#include "cutils.h"
#include <nuklear.h>
#include "game_ui.h"


static void
game_clamp_cam(vec3f_t* cam, const cg_map_t* cg_map, const ren_t* ren)
{
	if (cg_map == NULL)
		return;

	const cg_map_header_t* map = &cg_map->header;

	f32 offset = 20.0;

	const f32 max_x = (map->w * map->grid_size * ren->scale.x) - (ren->viewport.x - offset);
	const f32 max_y = (map->h * map->grid_size * ren->scale.y) - (ren->viewport.y - offset);

	cam->x = clampf(cam->x, -max_x, offset);
	cam->y = clampf(cam->y, -max_y, offset);
}

static void 
game_client_shoot(client_game_t* game)
{
	if (game->app->on_ui)
		return;

	const player_t* player = game->player;

	const vec2f_t mpos = screen_to_world(game->ren, &game->app->mouse);
	vec2f_t dir = vec2f(
		mpos.x - (player->core->pos.x + (player->core->size.x / 2)),
		mpos.y - (player->core->pos.y + (player->core->size.y / 2))
	);
	vec2f_norm(&dir);

	coregame_player_shoot(&game->cg, game->player->core, dir);

	net_udp_player_shoot_t* shoot = mmframes_alloc(&game->app->mmf, sizeof(net_udp_player_shoot_t));
	shoot->shoot_dir = dir;
	ssp_segbuff_add(&game->net->udp.buf, NET_UDP_PLAYER_SHOOT, sizeof(net_udp_player_shoot_t), shoot);
}

void
game_lock_cam(client_game_t* game)
{
	player_t* player = game->player;
	vec2f_t origin = vec2f(
		player->core->pos.x + (player->core->size.x / 2),
		player->core->pos.y + (player->core->size.y / 2)
	);

	const vec2f_t* viewport = &game->ren->viewport;
	game->app->cam.x = -(origin.x * game->ren->scale.x - (viewport->x / 2));
	game->app->cam.y = -(origin.y * game->ren->scale.y - (viewport->y / 2));

	game_clamp_cam(&game->app->cam, game->cg.map, game->ren);

	ren_set_view(game->ren, &game->app->cam);

	player->core->cursor = screen_to_world(game->ren, &game->app->mouse);
	ssp_segbuff_add(&game->net->udp.buf, NET_UDP_PLAYER_CURSOR, sizeof(vec2f_t), &player->core->cursor);
}

static void
game_handle_key(client_game_t* game, wa_window_t* window, const wa_event_key_t* ev)
{
    wa_state_t* state = wa_window_get_state(window);
	player_t* player = game->player;

	if (game->open_chat && ev->key != WA_KEY_ESC)
		return;

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
			if (ev->pressed && ((game->lock_cam = !game->lock_cam)))
				game_lock_cam(game);
			break;
		case WA_KEY_T:
			if (ev->pressed)
				game->trigger_shooting = !game->trigger_shooting;
			break;
		case WA_KEY_ENTER:
			if (ev->pressed)
				game->open_chat = true;
			break;
		case WA_KEY_ESC:
			if (ev->pressed)
				game->open_chat = false;
			break;
		default:
			break;
	}
}

static void 
game_handle_pointer(client_game_t* game, UNUSED const wa_event_pointer_t* ev)
{
	if (game->player)
	{
		game->player->core->cursor = screen_to_world(game->ren, &game->app->mouse);
		ssp_segbuff_add(&game->net->udp.buf, NET_UDP_PLAYER_CURSOR, sizeof(vec2f_t), &game->player->core->cursor);
	}
}

static void 
game_handle_mouse_button(client_game_t* game, const wa_event_mouse_t* ev)
{
	if (ev->button == WA_MOUSE_LEFT && ev->pressed)
		game_client_shoot(game);
}

void 
game_handle_mouse_wheel(waapp_t* app, const wa_event_wheel_t* ev)
{
	f32 dir = (ev->value > 0) ? -1.0 : 1.0;

	if (app->on_ui)
		return;

	vec2f_t old_wpos = screen_to_world(&app->ren, &app->mouse);

	if (dir > 0)
	{
		app->ren.scale.x += 0.1;
		app->ren.scale.y += 0.1;
	}
	else
	{
		app->ren.scale.x -= 0.1;
		app->ren.scale.y -= 0.1;
	}
	app->ren.scale.x = app->ren.scale.y = clampf(app->ren.scale.x, app->min_zoom, app->max_zoom);

	ren_set_scale(&app->ren, &app->ren.scale);

	shader_t* shader = &app->game->laser_bro->shader;
	shader_bind(shader);
	shader_uniform1f(shader, "scale", app->ren.scale.x);

	if (app->game && app->game->lock_cam)
		game_lock_cam(app->game);
	else
	{
		vec2f_t new_wpos = screen_to_world(&app->ren, &app->mouse);

		app->cam.x = app->cam.x - (old_wpos.x - new_wpos.x) * app->ren.scale.x;
		app->cam.y = app->cam.y - (old_wpos.y - new_wpos.y) * app->ren.scale.y;

		if (app->clamp_cam)
			game_clamp_cam(&app->cam, app->current_map, &app->ren);

		ren_set_view(&app->ren, &app->cam);
	}
}

static void 
on_player_free(cg_player_t* player, void* data)
{
	client_game_t* game = data;
	ght_del(&game->players, player->id);
}

static void 
game_update_logic(client_game_t* game)
{
	player_t* player = game->player;

	if (player)
	{
		coregame_set_player_dir(player->core, player->movement_dir);

		coregame_update(&game->cg);

		if (game->prev_pos.x != player->core->pos.x || game->prev_pos.y != player->core->pos.y)
		{
			if (game->lock_cam)
				game_lock_cam(game);
			game->prev_pos = player->core->pos;
		}
		if (game->prev_dir.x != player->core->dir.x || game->prev_dir.y != player->core->dir.y)
		{
			ssp_segbuff_add(&game->net->udp.buf, NET_UDP_PLAYER_DIR, sizeof(net_udp_player_dir_t), &player->core->dir);
			game->prev_dir = player->core->dir;
		}

		if (game->trigger_shooting)
			game_client_shoot(game);

		client_net_try_udp_flush(game->app);
	}

	game_move_cam(game->app);
}

void
game_add_chatmsg(client_game_t* game, const char* name, const char* msg)
{
	if (msg == NULL || *msg == 0x00)
		return;

	chatmsg_t* chatmsg = array_add_into(&game->chat_msgs);
	if (name)
		chatmsg->name_len = snprintf(chatmsg->name, PLAYER_NAME_MAX, "%s:", name);
	else
	{
		chatmsg->name[0] = 0x00;
		chatmsg->name_len = 0;
	}
	chatmsg->msg_len = snprintf(chatmsg->msg, CHAT_MSG_MAX, "%s", msg);

	if (game->chat_msgs.count >= 30)
		array_erase(&game->chat_msgs, 0);

	game->new_msg = true;
	game->last_chatmsg = (game->app->start_time.tv_nsec / 1e9) + game->app->start_time.tv_sec;
}

void 
game_send_chatmsg(client_game_t* game, const char* msg)
{
	if (*msg == 0x00)
		return;

	net_tcp_chat_msg_t* chatmsg = mmframes_alloc(&game->app->mmf, sizeof(net_tcp_chat_msg_t));
	strncpy(chatmsg->msg, msg, CHAT_MSG_MAX);

	ssp_segbuff_add(&game->net->tcp.buf, NET_TCP_CHAT_MSG, sizeof(net_tcp_chat_msg_t), chatmsg);
	ssp_tcp_send_segbuf(&game->net->tcp.sock, &game->net->tcp.buf);
}

static void
game_set_laser_thickness(client_game_t* game)
{
	shader_t* shader = &game->laser_bro->shader;
	shader_bind(shader);

	f32 lt_x = (game->laser_thicc_px / game->ren->viewport.x) * 2.0;
	f32 lt_y = (game->laser_thicc_px / game->ren->viewport.y) * 2.0;

	shader_uniform1f(shader, "line_thick", fminf(lt_x, lt_y));
}

void* 
game_init(waapp_t* app)
{
	client_game_t* game = calloc(1, sizeof(client_game_t));
	game->app = app;
	game->net = &app->net;
	game->ren = &app->ren;
	app->game = game;
	game->nk_ctx = app->nk_ctx;

	coregame_init(&game->cg, true, app->map_from_server);
	game->cg.user_data = game;
	game->cg.player_free_callback = on_player_free;

	game->tank_bottom_tex = texture_load("res/tank_bottom.png", TEXTURE_NEAREST);
	game->tank_bottom_tex->name = "Tank Bottom";
	game->tank_top_tex = texture_load("res/tank_top.png", TEXTURE_NEAREST);
	game->tank_top_tex->name = "Tank Top";
	game->lock_cam = true;

	ght_init(&game->players, 10, free);

	app->keybind.cam_move = WA_MOUSE_RIGHT;

	const cg_map_header_t* maph = &game->cg.map->header;

	rect_init(&app->map_border, vec2f(0, 0), 
		vec2f(
			maph->w * maph->grid_size,
			maph->h * maph->grid_size
		), 
		0xFF0000FF, 
		NULL
	);

	array_init(&game->player_deaths, sizeof(player_kill_t), 10);

	game->death_kill_time = 10.0;
	app->current_map = game->cg.map;

	array_init(&game->chat_msgs, sizeof(chatmsg_t), 10);

	const i32 layout[] = {
		VERTLAYOUT_F32, 2, // vertex position
		VERTLAYOUT_F32, 2, // position a
		VERTLAYOUT_F32, 2, // position b
		VERTLAYOUT_END
	};
	const bro_param_t param = {
		.draw_mode = DRAW_TRIANGLES,
		.max_vb_count = 4096,
		.vert_path = "client/src/shaders/laser_vert.glsl",
		.frag_path = "client/src/shaders/laser_frag.glsl",
		.shader = NULL,
		.vertlayout = layout,
		.vertex_size = sizeof(laser_vertex_t),
		.draw_misc = ren_laser_draw_misc,
		.draw_line = NULL, 
		.draw_rect = NULL
	};
	game->laser_bro = ren_new_bro(game->ren, &param);

	game->laser_thicc_px = 25.0;
	shader_t* shader = &game->laser_bro->shader;
	game_set_laser_thickness(game);
	shader_uniform1f(shader, "scale", game->ren->scale.x);

	return game;
}

void 
game_update(waapp_t* app, client_game_t* game)
{
	clock_gettime(CLOCK_MONOTONIC, &app->start_time);

	game_ui_update(game);
	game_update_logic(game);
	game_draw(game);
	app->frames++;

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
game_event(waapp_t* app, client_game_t* game, const wa_event_t* ev)
{
	wa_window_t* window = app->window;

	switch (ev->type)
	{
		case WA_EVENT_KEYBOARD:
			game_handle_key(game, window, &ev->keyboard);
			return 0;
		case WA_EVENT_POINTER:
			game_handle_pointer(game, &ev->pointer);
			return 0;
		case WA_EVENT_MOUSE_BUTTON:
			game_handle_mouse_button(game, &ev->mouse);
			return 0;
		case WA_EVENT_MOUSE_WHEEL:
			game_handle_mouse_wheel(app, &ev->wheel);
			return 0;
		case WA_EVENT_RESIZE:
			game_set_laser_thickness(game);
			return 0;
		default:
			return 1;
	}
}

void 
game_cleanup(waapp_t* app, client_game_t* game)
{
	ren_delete_bro(game->laser_bro);

	texture_del(game->tank_bottom_tex);
	texture_del(game->tank_top_tex);

	array_del(&game->player_deaths);
	array_del(&game->chat_msgs);
	ght_destroy(&game->players);
	coregame_cleanup(&game->cg);
	client_net_disconnect(app);
	game->player = NULL;
	app->game = NULL;

	free(game);
}

void
game_move_cam(waapp_t* app)
{
    wa_state_t* state = wa_window_get_state(app->window);
    vec3f_t* cam = &app->cam;

    if (app->mouse.x != app->mouse_prev.x || app->mouse.y != app->mouse_prev.y)
    {
        if (state->mouse_map[app->keybind.cam_move])
        {
			if (app->game)
				app->game->lock_cam = false;
            vec2f_t diff = vec2f(
                app->mouse_prev.x - app->mouse.x,
                app->mouse_prev.y - app->mouse.y
            );
			cam->x -= diff.x;
			cam->y -= diff.y;
			if (app->clamp_cam)
				game_clamp_cam(cam, app->current_map, &app->ren);
            ren_set_view(&app->ren, cam);
        }

        app->mouse_prev = app->mouse;
    }
}

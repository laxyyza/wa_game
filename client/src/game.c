#include "game.h"
#include "game_draw.h"
#include "app.h"
#include "cutils.h"
#include <nuklear.h>
#include "game_ui.h"

void
game_update_ui_bars_pos(client_game_t* game)
{
	progress_bar_set_pos(&game->health_bar, 
		vec2f(
			game->ren->viewport.x - (game->health_bar.background.size.x + 10),
			game->ren->viewport.y - (game->health_bar.background.size.y + 10)
		)
	);

	progress_bar_set_pos(&game->guncharge_bar, 
		vec2f(
			game->health_bar.background.pos.x,
			game->health_bar.background.pos.y - game->health_bar.background.size.y
		)
	);
}

static void
game_clamp_cam(vec3f_t* cam, const cg_runtime_map_t* map, const ren_t* ren)
{
	if (map == NULL)
		return;

	f32 offset = 20.0;

	const f32 max_x = (map->w * map->grid_size * ren->scale.x) - (ren->viewport.x - offset);
	const f32 max_y = (map->h * map->grid_size * ren->scale.y) - (ren->viewport.y - offset);

	cam->x = clampf(cam->x, -max_x, offset);
	cam->y = clampf(cam->y, -max_y, offset);
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
	ssp_segbuf_add(&game->net->udp.buf, NET_UDP_PLAYER_CURSOR, sizeof(vec2f_t), &player->core->cursor);
}

static void
game_handle_num_keys(client_game_t* game, const wa_event_key_t* ev)
{
	if (ev->pressed == false)
		return;

	enum cg_gun_id gun_id;

	if (ev->key == WA_KEY_1)
		gun_id = CG_GUN_ID_SMALL;
	else if (ev->key == WA_KEY_2)
		gun_id = CG_GUN_ID_BIG;
	else if (ev->key == WA_KEY_3)
		gun_id = CG_GUN_ID_MINI_GUN;
	else
		return;
	
	if (coregame_player_change_gun(&game->cg, game->player->core, gun_id))
	{
		u32* udp_gun_id = mmframes_alloc(&game->app->mmf, sizeof(u32));
		*udp_gun_id = gun_id;
		ssp_segbuf_add_i(&game->net->udp.buf, NET_UDP_PLAYER_GUN_ID, sizeof(u32), udp_gun_id);
	}
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
				player->input |= PLAYER_INPUT_UP;
			else
				player->input ^= PLAYER_INPUT_UP;
			break;
		case WA_KEY_S:
			if (ev->pressed)
				player->input |= PLAYER_INPUT_DOWN;
			else
				player->input ^= PLAYER_INPUT_DOWN;
			break;
		case WA_KEY_A:
			if (ev->pressed)
				player->input |= PLAYER_INPUT_LEFT;
			else
				player->input ^= PLAYER_INPUT_LEFT;
			break;
		case WA_KEY_D:
			if (ev->pressed)
				player->input |= PLAYER_INPUT_RIGHT;
			else
				player->input ^= PLAYER_INPUT_RIGHT;
			break;
		case WA_KEY_SPACE:
			if (ev->pressed && ((game->lock_cam = !game->lock_cam)))
				game_lock_cam(game);
			break;
		case WA_KEY_T:
			if (ev->pressed)
				game->player->input ^= PLAYER_INPUT_SHOOT;
			break;
		case WA_KEY_ENTER:
			if (ev->pressed)
				game->open_chat = true;
			break;
		case WA_KEY_ESC:
			if (ev->pressed)
				game->open_chat = false;
			break;
		case WA_KEY_1:
		case WA_KEY_2:
		case WA_KEY_3:
			game_handle_num_keys(game, ev);
			break;
		case WA_KEY_R:
			coregame_player_reload(&game->cg, game->player->core);
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
		ssp_segbuf_add(&game->net->udp.buf, NET_UDP_PLAYER_CURSOR, sizeof(vec2f_t), &game->player->core->cursor);
	}
}

static void 
game_handle_mouse_button(client_game_t* game, const wa_event_mouse_t* ev)
{
	if (ev->button == WA_MOUSE_LEFT)
	{
		if (ev->pressed)
			game->player->input |= PLAYER_INPUT_SHOOT;
		else if (game->player->input & PLAYER_INPUT_SHOOT)
			game->player->input ^= PLAYER_INPUT_SHOOT;
	}
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

	if (app->game)
	{
		shader_t* shader = &app->game->laser_bro->shader;
		shader_bind(shader);
		shader_uniform1f(shader, "scale", app->ren.scale.x);
	}

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
on_player_free(cg_player_t* player, UNUSED void* data)
{
	free(player->user_data);
}

static void 
game_update_logic(client_game_t* game)
{
	player_t* player = game->player;
	if (player == NULL)
		return;

	coregame_set_player_input(player->core, player->input);

	coregame_update(&game->cg);
	progress_bar_update(&game->health_bar);
	player_update_guncharge(game->player, &game->guncharge_bar);

	if (game->prev_pos.x != player->core->pos.x || game->prev_pos.y != player->core->pos.y)
	{
		if (game->lock_cam)
			game_lock_cam(game);
		game->prev_pos = player->core->pos;
	}
	if (game->player->input != game->prev_input)
	{
		ssp_segbuf_add_i(&game->net->udp.buf, NET_UDP_PLAYER_INPUT, sizeof(u8), &player->input);
		game->prev_input = player->input;
	}

	client_net_try_udp_flush(game->app);

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
	strncpy(chatmsg->msg, msg, CHAT_MSG_MAX - 1);

	ssp_segbuf_add(&game->net->tcp.buf, NET_TCP_CHAT_MSG, sizeof(net_tcp_chat_msg_t), chatmsg);
	ssp_tcp_send_segbuf(&game->net->tcp.sock, &game->net->tcp.buf);
}

static void 
game_set_scale_laser_thickness(const client_game_t* game, laser_bullet_t* bullet)
{
	const f32 lt_x = (bullet->thickness / game->ren->viewport.x) * 2.0;
	const f32 lt_y = (bullet->thickness / game->ren->viewport.y) * 2.0;

	bullet->thickness_scaled = fminf(lt_x, lt_y);
}

static void
game_set_laser_thickness(client_game_t* game)
{
	game_set_scale_laser_thickness(game, &game->small_laser);
	game_set_scale_laser_thickness(game, &game->big_laser);

	game_update_ui_bars_pos(game);
}

static void 
game_init_add_gun_specs(waapp_t* app, client_game_t* game)
{
	if (app->tmp_gun_specs == NULL)
		return;

	const cg_gun_spec_t* gun_specs = (const cg_gun_spec_t*)app->tmp_gun_specs->buf;
	for (u32 i = 0; i < app->tmp_gun_specs->count; i++)
		coregame_add_gun_spec(&game->cg, gun_specs + i);

	array_del(app->tmp_gun_specs);
	free(app->tmp_gun_specs);
	app->tmp_gun_specs = NULL;
}

static void
game_load_gun_textures(client_game_t* game)
{
	const char* texture_paths[CG_GUN_ID_TOTAL] = {
		"res/default_gun.png",
		"res/big_gun.png",
		"res/mini_gun.png",
	};

	for (u32 i = 0; i < CG_GUN_ID_TOTAL; i++)
	{
		game->gun_textures[i] = texture_load(texture_paths[i], TEXTURE_NEAREST);
		game->gun_textures[i]->name = texture_paths[i];
	}
}

static void
game_on_bullet_create(cg_bullet_t* bullet, client_game_t* game)
{
	if (bullet->gun_id == CG_GUN_ID_SMALL || bullet->gun_id == CG_GUN_ID_MINI_GUN)
		bullet->data = &game->small_laser;
	else if (bullet->gun_id == CG_GUN_ID_BIG)
		bullet->data = &game->big_laser;
}

static void
game_on_player_reload(cg_player_t* player, client_game_t* game)
{
	if (player->id != game->player->core->id)
		return;

	ssp_segbuf_add_i(&game->net->udp.buf, NET_UDP_PLAYER_RELOAD, 0, NULL);
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
	game_init_add_gun_specs(app, game);
	game->cg.user_data = game;
	game->cg.player_free_callback = on_player_free;
	game->cg.on_bullet_create = (cg_bullet_create_callback_t)game_on_bullet_create;
	game->cg.player_reload = (cg_player_reload_callback_t)game_on_player_reload;

	game->tank_bottom_tex = texture_load("res/tank_bottom.png", TEXTURE_NEAREST);
	game->tank_bottom_tex->name = "Tank Bottom";

	game_load_gun_textures(game);

	game->lock_cam = true;

	app->keybind.cam_move = WA_MOUSE_RIGHT;

	const cg_runtime_map_t* map = game->cg.map;

	rect_init(&app->map_border, vec2f(0, 0), 
		vec2f(
			map->w * map->grid_size,
			map->h * map->grid_size
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
		VERTLAYOUT_F32, 1, // laser thickness
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

	game->small_laser.thickness = 25.0;
	game->small_laser.len = 60.0;

	game->big_laser.thickness = 400.0;
	game->big_laser.len = 250.0;

	game_set_laser_thickness(game);

	shader_t* shader = &game->laser_bro->shader;
	shader_bind(shader);
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
	ren_delete_bro(game->ren, game->laser_bro);

	texture_del(game->tank_bottom_tex);
	for (u32 i = 0; i < CG_GUN_ID_TOTAL; i++)
		texture_del(game->gun_textures[i]);

	array_del(&game->player_deaths);
	array_del(&game->chat_msgs);
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

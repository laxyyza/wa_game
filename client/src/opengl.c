#define _GNU_SOURCE
#include "opengl.h"
#include <stdio.h>
#include "gui/gui.h"
#include <nuklear.h>
#include <string.h>
#include "client_net.h"

static bool failed = false;

static void
opengl_debug_callback(GLenum src, GLenum type, GLuint id,
                      GLenum severity, GLsizei len, const char* msg,
                      const void* data)
{
    waapp_t* app = (waapp_t*)data;
    wa_window_stop(app->window);
    failed = true;

    fprintf(stderr, "OpenGL Debug Message:\n");
    fprintf(stderr, "\tSource: 0x%x\n", src);
    fprintf(stderr, "\tType: 0x%x\n", type);
    fprintf(stderr, "\tID: %u\n", id);
    fprintf(stderr, "\tSeverity: 0x%x\n", severity);
    fprintf(stderr, "\tMessage (%u): '%s'\n", len, msg);
    fprintf(stderr, "-------------------------------------\n");
}


static void
waapp_enable_debug(waapp_t* app)
{
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(opengl_debug_callback, app);
}

static void
print_gl_version(void)
{
    info("OpenGL vendor: %s\n",
           glGetString(GL_VENDOR));
    info("OpenGL renderer: %s\n",
           glGetString(GL_RENDERER));
    info("OpenGL version: %s\n",
           glGetString(GL_VERSION));
    info("OpenGL GLSL version: %s\n",
           glGetString(GL_SHADING_LANGUAGE_VERSION));
}

bool
waapp_opengl_init(waapp_t* app)
{
    gladLoadGL();
    waapp_enable_debug(app);
    print_gl_version();
	glLineWidth(2.0);

    ren_init(&app->ren);

    wa_state_t* state = wa_window_get_state(app->window);
    ren_viewport(&app->ren, state->window.w, state->window.h);

    gui_init(app);

    ren_set_view(&app->ren, &app->cam);

    return !failed;
}

void
waapp_opengl_cleanup(waapp_t* app)
{
    ren_del(&app->ren);
}

static void
ui_kills_window(waapp_t* app, struct nk_context* ctx)
{
	array_t* player_deaths = &app->player_deaths;
	struct timespec current_time;
	u32 delete_count = 0;

	if (player_deaths->count == 0)
		return;

	player_kill_t* kills = (player_kill_t*)player_deaths->buf;
	player_kill_t* kill;

	clock_gettime(CLOCK_MONOTONIC, &current_time);

    static struct nk_rect kill_window_rect = {
        .w = 500,
        .h = 200
    };
	kill_window_rect.x = (app->ren.viewport.x - kill_window_rect.w);
	static nk_flags flags = NK_WINDOW_NOT_INTERACTIVE | NK_WINDOW_BACKGROUND | NK_WINDOW_NO_SCROLLBAR;

	if (nk_begin(ctx, "kills", kill_window_rect, flags))
	{
		nk_layout_row_dynamic(ctx, 30, 1);

		for (u32 i = 0; i < player_deaths->count; i++)
		{
			kill = kills + i;
			char label[256];
			snprintf(label, 256, "%s -> %s", kill->attacker_name, kill->target_name); 
			nk_label(ctx, label, NK_TEXT_RIGHT);

			f64 elapsed_time = get_elapsed_time(&current_time, &kill->timestamp);

			if (elapsed_time >= app->death_kill_time)
				delete_count++;
		}
	}
	nk_end(ctx);

	for (u32 i = 0; i < delete_count; i++)
		array_erase(player_deaths, 0);
}

static void
ui_score_window(waapp_t* app, struct nk_context* ctx)
{
	static struct nk_rect score_window_rect = {
		.w = 200,
		.h = 50,
		.y = 0
	};
	score_window_rect.x = (app->ren.viewport.x / 2 - (score_window_rect.w / 2));

	if (nk_begin(ctx, "score", score_window_rect, NK_WINDOW_NOT_INTERACTIVE | NK_WINDOW_NO_SCROLLBAR))
	{
		nk_layout_row_dynamic(ctx, score_window_rect.h, 1);
		char label[64];
		snprintf(label, 64, "Kills: %u", app->player->core->stats.kills);
		nk_label(ctx, label, NK_TEXT_CENTERED);
	}
	nk_end(ctx);
}

static void 
ui_tab_display_player(struct nk_context* ctx, cg_player_t* player)
{
	char player_stats[64];
	nk_label(ctx, player->username, NK_TEXT_CENTERED);

	snprintf(player_stats, 64, "%u", player->stats.kills);
	nk_label(ctx, player_stats, NK_TEXT_CENTERED);

	snprintf(player_stats, 64, "%u", player->stats.deaths);
	nk_label(ctx, player_stats, NK_TEXT_CENTERED);

	snprintf(player_stats, 64, "%.2f", player->stats.ping);
	nk_label(ctx, player_stats, NK_TEXT_CENTERED);
}

static f32
percent(f32 per, f32 max)
{
	return (per / 100.0) * max;
}

static void
ui_tab_window(waapp_t* app, struct nk_context* ctx)
{
	wa_state_t* state = wa_window_get_state(app->window);
	if (state->key_map[WA_KEY_TAB] == 0)
		return;

	struct nk_rect rect = {
		.x = percent(20, app->ren.viewport.x),
		.y = percent(20, app->ren.viewport.y),
	};
	rect.w = percent(70, app->ren.viewport.x) - rect.x / 2;
	rect.h = percent(70, app->ren.viewport.y) - rect.y / 2;
	const ght_t* players = &app->game.players;

	if (nk_begin(ctx, "Scoreboard", rect, NK_WINDOW_TITLE))
	{
		nk_layout_row_dynamic(ctx, 50, 4);
		
		nk_label(ctx, "PLAYERS", NK_TEXT_CENTERED);
		nk_label(ctx, "KILLS", NK_TEXT_CENTERED);
		nk_label(ctx, "DEATHS", NK_TEXT_CENTERED);
		nk_label(ctx, "PING", NK_TEXT_CENTERED);

		nk_layout_row_dynamic(ctx, 20, 4);

		ui_tab_display_player(ctx, app->player->core);

		GHT_FOREACH(cg_player_t* player, players, {
			if (player->id != app->player->core->id)
				ui_tab_display_player(ctx, player);
		});
	}
	nk_end(ctx);
}

static void
waapp_gui(waapp_t* app)
{
    struct nk_context* ctx = app->nk_ctx;
    struct nk_colorf bg;
    memcpy(&bg, &app->bg_color, sizeof(struct nk_colorf));
    wa_state_t* state = wa_window_get_state(app->window);

    static nk_flags win_flags = 
        NK_WINDOW_BORDER | NK_WINDOW_MINIMIZABLE | 
         NK_WINDOW_TITLE | NK_WINDOW_MINIMIZED |
        NK_WINDOW_CLOSABLE | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE;
    const char* window_name = state->window.title;
    // static struct nk_vec2 window_offset = {10, 10};
    static struct nk_rect window_rect = {
        .x = 0,
        .y = 0,
        .w = 340,
        .h = 500
    };

    if (nk_begin(ctx, window_name, window_rect, win_flags))
                 
    {
		char udp_in_stat[256];
        if (win_flags & NK_WINDOW_MINIMIZED)
            win_flags ^= NK_WINDOW_MINIMIZED;

		snprintf(udp_in_stat, 256, "Game Server: %s:%u (%.1f/s)",
				app->net.udp.ipaddr, app->net.udp.port, app->net.udp.tickrate);
        nk_layout_row_dynamic(ctx, 20, 1);
		nk_label(ctx, udp_in_stat, NK_TEXT_LEFT);

		if (app->net.udp.latency < 1.0)
			snprintf(udp_in_stat, 256, "PING: %.4f ms", app->net.udp.latency);
		else
			snprintf(udp_in_stat, 256, "PING: %.2f ms", app->net.udp.latency);
		nk_label(ctx, udp_in_stat, NK_TEXT_LEFT);

		snprintf(udp_in_stat, 256, "CLIENT FPS: %u (%.2f ms)", app->fps, app->frame_time);
		nk_label(ctx, udp_in_stat, NK_TEXT_LEFT);

		wa_state_t* state = wa_window_get_state(app->window);
		nk_bool vsync = !state->window.vsync;
		if (nk_checkbox_label(ctx, "VSync", &vsync))
		{
			app->update_vync = true;
			app->tmp_vsync = !vsync;
		}

        nk_layout_row_dynamic(ctx, 20, 1);
		if (vsync)
		{
			nk_bool limit_fps = !app->fps_limit;
			if (nk_checkbox_label(ctx, "Limit FPS", &limit_fps))
				app->fps_limit = !limit_fps;

			if (app->fps_limit)
			{
				snprintf(udp_in_stat, 256, "FPS LIMIT: %u", (u32)app->max_fps);
				nk_label(ctx, udp_in_stat, NK_TEXT_LEFT);
				if (nk_slider_float(ctx, 10.0, &app->max_fps, 500.0, 1.0))
					waapp_set_max_fps(app, app->max_fps);
			}
		}
        const char* fullscreen_str = (state->window.state & WA_STATE_FULLSCREEN) ? "Windowed" : "Fullscreen";
        if (nk_button_label(ctx, fullscreen_str))
            wa_window_set_fullscreen(app->window, !(state->window.state & WA_STATE_FULLSCREEN));

		snprintf(udp_in_stat, 256, "IN  UDP Packets/s: %u (%zu bytes)", 
				app->net.udp.in.last_count, app->net.udp.in.last_bytes);
		nk_label(ctx, udp_in_stat, NK_TEXT_LEFT);

		snprintf(udp_in_stat, 256, "OUT UDP Packets/s: %u (%zu bytes)", 
				app->net.udp.out.last_count, app->net.udp.out.last_bytes);
		nk_label(ctx, udp_in_stat, NK_TEXT_LEFT);

		snprintf(udp_in_stat, 256, "Interpolate Factor: %f", app->game.interp_factor);
		nk_label(ctx, udp_in_stat, NK_TEXT_LEFT);
		nk_slider_float(ctx, 0.01, &app->game.interp_factor, 1.0, 0.005);

		snprintf(udp_in_stat, 256, "Interp Threshold Dist: %f", app->game.interp_threshold_dist);
		nk_label(ctx, udp_in_stat, NK_TEXT_LEFT);
		nk_slider_float(ctx, 0.0001, &app->game.interp_threshold_dist, 20.0, 0.0001);

		snprintf(udp_in_stat, 256, "Draw calls: %u", app->ren.draw_calls);
		nk_label(ctx, udp_in_stat, NK_TEXT_LEFT);

		if (nk_button_label(ctx, "Main Menu"))
		{
			waapp_state_switch(app, &app->sm.states.main_menu);
		}

    }
    nk_end(ctx);
	if (nk_window_is_hidden(ctx, window_name))
		wa_window_stop(app->window);

	gui_set_font(app, app->font_big);
	struct nk_style_item og_bg = ctx->style.window.fixed_background;
	ctx->style.window.fixed_background = nk_style_item_color(nk_rgba(0, 0, 0, 0));

	ui_kills_window(app, ctx);
	ui_score_window(app, ctx);

	ctx->style.window.fixed_background = og_bg;
	gui_set_font(app, app->font);

	ui_tab_window(app, ctx);
}

static void 
render_player(waapp_t* app, player_t* player)
{
	healthbar_t* hpbar = &player->hpbar;

	player->rect.pos = player->core->pos;
	player->top.pos = player->core->pos;

	player->hpbar.background.pos = vec2f(
		player->rect.pos.x + (player->rect.size.x - player->hpbar.background.size.x) / 2,
		player->rect.pos.y - 30
	);
	player->hpbar.fill.pos = hpbar->background.pos; 

	ren_draw_rect(&app->ren, &player->rect);
	ren_draw_rect(&app->ren, &player->top);

	ren_draw_rect(&app->ren, &player->hpbar.background);
	ren_draw_rect(&app->ren, &player->hpbar.fill);
}

static void
render_projectiles(waapp_t* app)
{
	cg_projectile_t* proj = app->game.proj.head;

	while (proj)
	{
		rect_t rect;
		rect_init(&rect, proj->rect.pos, proj->rect.size, 0xFF0000FF, NULL);
		rect.rotation = proj->rotation;
		ren_draw_rect(&app->ren, &rect);

		proj = proj->next;
	}
}

static void
render_players(waapp_t* app)
{
	const ght_t* players = &app->players;
	GHT_FOREACH(player_t* player, players, {
		if (player->core->dir.x || player->core->dir.y)
			player->rect.rotation = atan2(player->core->dir.y, player->core->dir.x) + M_PI / 2;

		const vec2f_t origin = rect_origin(&player->rect);
		player->top.rotation = angle(&origin, &player->core->cursor);
		render_player(app, player);
	});
}

static void
render_cell(waapp_t* app, const cg_map_t* map, const cg_cell_t* cell)
{
	const u32 grid_size = map->header.grid_size;
	ren_t* ren = &app->ren;
	texture_t* texture = NULL;
	rect_t cell_rect = {0};

	if (cell->type == CG_CELL_EMPTY)
		texture = app->grass_tex;
	else if (cell->type == CG_CELL_BLOCK)
		texture = app->block_tex;

	rect_init(&cell_rect, 
		vec2f(cell->pos.x * grid_size, cell->pos.y * grid_size), 
		vec2f(grid_size, grid_size), 0x000000FF, texture);
	ren_draw_rect(ren, &cell_rect);
}

static void
render_grid(waapp_t* app, u32 w, u32 h, u32 cell_size_w, u32 cell_size_h)
{
	ren_t* ren = &app->ren;
	vec2f_t start;
	vec2f_t end;

	u32 line_color = 0x000000FF;

	for (u32 x = 1; x < w; x++)
	{
		start = vec2f(x * cell_size_w, 0);
		end	  = vec2f(start.x, h * cell_size_h);

		ren_draw_line(ren, &start, &end, line_color);
	}

	for (u32 y = 1; y < h; y++)
	{
		start = vec2f(0,	y * cell_size_h);
		end   =	vec2f(w * cell_size_w, start.y);

		ren_draw_line(ren, &start, &end, line_color);
	}
}

void
waapp_render_map(waapp_t* app, cg_map_t* map, bool show_grid)
{
	ren_t* ren = &app->ren;
	const vec2f_t cam = vec2f(-app->cam.x / ren->scale.x, -app->cam.y / ren->scale.y);
	const vec2f_t* viewport = &app->ren.viewport;
	const vec2f_t bot_right = vec2f(cam.x + viewport->x / ren->scale.x, cam.y + viewport->y / ren->scale.y);
	const cg_cell_t* c_left;
	const cg_cell_t* c_right;

	c_left = cg_map_at_wpos_clamp(map, &cam);
	c_right = cg_map_at_wpos_clamp(map, &bot_right);

	for (u32 x = c_left->pos.x; x <= c_right->pos.x; x++)
		for (u32 y = c_left->pos.y; y <= c_right->pos.y; y++)
			render_cell(app, map, cg_map_at(map, x, y));

	ren_bind_bro(ren, app->line_bro);

	if (show_grid)
		render_grid(app, map->header.w, map->header.h, map->header.grid_size, map->header.grid_size);
	ren_draw_rect(&app->ren, &app->map_border);

	ren_bind_bro(ren, app->ren.default_bro);
}

void
waapp_opengl_draw(waapp_t* app)
{
    ren_t* ren = &app->ren;

    ren_bind_bro(ren, ren->default_bro);

	waapp_move_cam(app);

	waapp_render_map(app, app->game.map, false);

	render_projectiles(app);
	render_players(app);

    ren_draw_batch(ren);

	ren_bind_bro(ren, app->line_bro);
    ren_draw_batch(ren);

	// ren_bind_bro(ren, app->line_bro);
	// ren_draw_rect(ren, &app->world_border);
	// ren_draw_batch(ren);

    waapp_gui(app);
}

u64
gl_sizeof(u32 type)
{
    switch (type)
    {
        case GL_FLOAT:
            return sizeof(GLfloat);
        case GL_INT:
        case GL_UNSIGNED_INT:
            return sizeof(GLuint);
        case GL_UNSIGNED_BYTE:
        case GL_BYTE:
            return sizeof(GLubyte);
        default:
            assert(false);
            return 0;
    }
}

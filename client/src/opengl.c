#include "opengl.h"
#include <stdio.h>
#include "gui/gui.h"
#include <nuklear.h>
#include <string.h>

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
    printf("OpenGL vendor: %s\n",
           glGetString(GL_VENDOR));
    printf("OpenGL renderer: %s\n",
           glGetString(GL_RENDERER));
    printf("OpenGL version: %s\n",
           glGetString(GL_VERSION));
    printf("OpenGL GLSL version: %s\n",
           glGetString(GL_SHADING_LANGUAGE_VERSION));
}

bool
waapp_opengl_init(waapp_t* app)
{
    gladLoadGL();
    waapp_enable_debug(app);
    print_gl_version();

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
waapp_gui(waapp_t* app)
{
    struct nk_context* ctx = app->nk_ctx;
    struct nk_colorf bg;
    memcpy(&bg, &app->bg_color, sizeof(struct nk_colorf));
    wa_state_t* state = wa_window_get_state(app->window);

    static nk_flags win_flags = 
        NK_WINDOW_BORDER | NK_WINDOW_MINIMIZABLE | 
         NK_WINDOW_TITLE | 
        NK_WINDOW_CLOSABLE | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE;
    const char* window_name = state->window.title;
    // static struct nk_vec2 window_offset = {10, 10};
    static struct nk_rect window_rect = {
        .x = 0,
        .y = 0,
        .w = 300,
        .h = 300
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

        nk_label(ctx, "Background Color", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 25, 1);
        if (nk_combo_begin_color(ctx, nk_rgb_cf(bg), nk_vec2(nk_widget_width(ctx), 400)))
        {
            nk_layout_row_dynamic(ctx, 200, 1);
            bg = nk_color_picker(ctx, bg, NK_RGBA);
            nk_layout_row_dynamic(ctx, 25, 1);
            bg.r = nk_propertyf(ctx, "#R", 0, bg.r, 1.0f, 0.01f, 0.005f);
            bg.g = nk_propertyf(ctx, "#G", 0, bg.g, 1.0f, 0.01f, 0.005f);
            bg.b = nk_propertyf(ctx, "#B", 0, bg.b, 1.0f, 0.01f, 0.005f);
            bg.a = nk_propertyf(ctx, "#A", 0, bg.a, 1.0f, 0.01f, 0.005f);
            nk_combo_end(ctx);
            memcpy(&app->bg_color, &bg, sizeof(vec4f_t));
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

    }
    nk_end(ctx);
    if (nk_window_is_hidden(ctx, window_name))
    {
        wa_window_stop(app->window);
        printf("CLOSE!\n");
    }
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
render_grid(waapp_t* app, u32 w, u32 h, u32 cell_size_w, u32 cell_size_h, bool show_grid)
{
	ren_t* ren = &app->ren;
	vec2f_t start;
	vec2f_t end;

	ren_bind_bro(ren, app->line_bro);
	u32 line_color = 0x000000FF;
	u32 edge_color = 0xFF0000FF;
	u32 color;

	for (u32 x = 0; x <= w; x++)
	{
		start = vec2f(x * cell_size_w, 0);
		end	  = vec2f(start.x, h * cell_size_h);

		if (x == 0 || x == w)
			color = edge_color;
		else if (show_grid == false)
			continue;
		else
			color = line_color;

		ren_draw_line(ren, &start, &end, color);
	}

	for (u32 y = 0; y <= h; y++)
	{
		start = vec2f(0,	y * cell_size_h);
		end   =	vec2f(w * cell_size_w, start.y);

		if (y == 0 || y == h)
			color = edge_color;
		else if (show_grid == false)
			continue;
		else
			color = line_color;
		
		ren_draw_line(ren, &start, &end, color);
	}
	ren_draw_batch(ren);
	ren_bind_bro(ren, app->ren.default_bro);
}

void
waapp_render_map(waapp_t* app, cg_map_t* map, UNUSED bool show_grid)
{
	for (u32 x = 0; x < map->header.w; x++)
		for (u32 y = 0; y < map->header.h; y++)
			render_cell(app, map, cg_map_at(map, x, y));

	render_grid(app, map->header.w, map->header.h, map->header.grid_size, map->header.grid_size, show_grid);
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

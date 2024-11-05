#include "game_draw.h"
#include "util.h"
#include "app.h"
#include "game_ui.h"

static void 
game_render_player(ren_t* ren, player_t* player)
{
	healthbar_t* hpbar = &player->hpbar;

	player->rect.pos = player->core->pos;
	player->top.pos = player->core->pos;

	player->hpbar.background.pos = vec2f(
		player->rect.pos.x + (player->rect.size.x - player->hpbar.background.size.x) / 2,
		player->rect.pos.y - 30
	);
	player->hpbar.fill.pos = hpbar->background.pos; 

	ren_draw_rect(ren, &player->rect);
	ren_draw_rect(ren, &player->top);

	ren_draw_rect(ren, &player->hpbar.background);
	ren_draw_rect(ren, &player->hpbar.fill);
}

static void
game_render_projectiles(client_game_t* game)
{
	cg_projectile_t* proj = game->cg.proj.head;

	while (proj)
	{
		rect_t rect;
		rect_init(&rect, proj->rect.pos, proj->rect.size, 0xFF0000FF, NULL);
		rect.rotation = proj->rotation;
		ren_draw_rect(game->ren, &rect);

		proj = proj->next;
	}
}

static void
game_render_players(client_game_t* game)
{
	const ght_t* players = &game->players;
	GHT_FOREACH(player_t* player, players, {
		if (player->core->dir.x || player->core->dir.y)
			player->rect.rotation = atan2(player->core->dir.y, player->core->dir.x) + M_PI / 2;

		const vec2f_t origin = rect_origin(&player->rect);
		player->top.rotation = angle(&origin, &player->core->cursor);
		game_render_player(game->ren, player);
	});
}

static void
game_render_cell(waapp_t* app, const cg_map_t* map, const cg_cell_t* cell)
{
	const u32 grid_size = map->header.grid_size;
	ren_t* ren = &app->ren;
	texture_t* texture = NULL;
	rect_t cell_rect = {0};
	u32 color = 0;

	if (cell->type == CG_CELL_EMPTY)
		texture = app->grass_tex;
	else if (cell->type == CG_CELL_BLOCK)
		texture = app->block_tex;
	else if (cell->type == CG_CELL_SPAWN)
		color = 0x000066FF;

	rect_init(&cell_rect, 
		vec2f(cell->pos.x * grid_size, cell->pos.y * grid_size), 
		vec2f(grid_size, grid_size), color, texture);
	ren_draw_rect(ren, &cell_rect);
}

static void
game_render_grid(ren_t* ren, u32 w, u32 h, u32 cell_size_w, u32 cell_size_h)
{
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
game_render_map(waapp_t* app, cg_map_t* map, bool show_grid)
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
			game_render_cell(app, map, cg_map_at(map, x, y));

	ren_bind_bro(ren, app->line_bro);

	if (show_grid)
		game_render_grid(ren, map->header.w, map->header.h, map->header.grid_size, map->header.grid_size);
	ren_draw_rect(&app->ren, &app->map_border);

	ren_bind_bro(ren, app->ren.default_bro);
}


void 
game_draw(client_game_t* game)
{
	ren_t* ren = game->ren;
	waapp_t* app = game->app;

	ren_bind_bro(ren, ren->default_bro);

	game_move_cam(app);

	game_render_map(app, game->cg.map, false);

	game_render_projectiles(game);
	game_render_players(game);

    ren_draw_batch(ren);

	ren_bind_bro(ren, app->line_bro);
    ren_draw_batch(ren);

	game_ui_update(game);
}

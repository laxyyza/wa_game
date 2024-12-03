#include "game_draw.h"
#include "util.h"
#include "app.h"
#include "game_ui.h"

static void 
game_render_progress_bar(ren_t* ren, bro_t* bro, const progress_bar_t* bar)
{
	bro->draw_rect(ren, bro, &bar->background);
	bro->draw_rect(ren, bro, &bar->fill);
}

static void 
game_render_player_body(ren_t* ren, player_t* player)
{
	ren_draw_rect(ren, &player->rect);

	if (player->gun_rect.texture)
		ren_draw_rect(ren, &player->gun_rect);
}

static void 
game_render_player(ren_t* ren, player_t* player)
{
	progress_bar_t* hpbar = &player->hpbar;

	player->rect.pos = player->core->pos;
	player->gun_rect.pos = vec2f(
		player->rect.pos.x - ((player->gun_rect.size.x - player->rect.size.x) / 2),
		player->rect.pos.y - ((player->gun_rect.size.y - player->rect.size.y) / 2)
	);

	player_update_guncharge(player, NULL);
	progress_bar_update_pos(hpbar);

	game_render_progress_bar(ren, ren->default_bro, hpbar);
	game_render_progress_bar(ren, ren->default_bro, &player->guncharge);

	game_render_player_body(ren, player);
}

static void
game_render_bullet_cells(client_game_t* game, const cg_bullet_t* bullet)
{
	const array_t* cells = &bullet->cells;
	const u32 grid_size = game->cg.map->grid_size;

	for (u32 i = 0; i < cells->count; i++)
	{
		const cg_runtime_cell_t* cell = ((const cg_runtime_cell_t**)cells->buf)[i];
		rect_t r = {
			.pos = vec2f(cell->pos.x * grid_size, cell->pos.y * grid_size),
			.size = vec2f(grid_size, grid_size),
			.color = rgba((cell->type == CG_CELL_BLOCK) ? 0xFF000066 : 0xFFFFFF22)
		};
		game->ren->default_bro->draw_rect(game->ren, game->ren->default_bro, &r);
	}
}

static void
game_render_bullet_debug(client_game_t* game, const cg_bullet_t* bullet)
{
	game_render_bullet_cells(game, bullet);

	if (bullet->collided)
	{
		rect_t r = {
			.size = vec2f(25, 25),
		};
		r.pos.x = bullet->contact_point.x - (r.size.x / 2);
		r.pos.y = bullet->contact_point.y - (r.size.y / 2);
		r.color = rgba(0xFF000080);
		game->ren->default_bro->draw_rect(game->ren, game->ren->default_bro, &r);

		vec2f_t a = vec2f(r.pos.x + (r.size.x / 2), r.pos.y);
		vec2f_t b = vec2f(a.x, a.y + r.size.y);
		game->ren->line_bro->draw_line(game->ren, game->ren->line_bro, &a, &b, 0x000000FF);
		a.x = r.pos.x;
		a.y = r.pos.y + (r.size.y / 2);
		b.x = a.x + r.size.x;
		b.y = a.y;
		game->ren->line_bro->draw_line(game->ren, game->ren->line_bro, &a, &b, 0x000000FF);
	}
}

static void
game_render_bullets(client_game_t* game)
{
	ght_t* bullets = &game->cg.bullets;
	GHT_FOREACH(const cg_bullet_t* bullet, bullets, 
	{
		const laser_bullet_t* bullet_data = bullet->data;
		laser_draw_data_t draw_data;
		draw_data.v.pos_a = bullet->r.pos;
		draw_data.laser_data = bullet_data;
		draw_data.v.pos_b.x = draw_data.v.pos_a.x + bullet_data->len * -bullet->dir.x;
		draw_data.v.pos_b.y = draw_data.v.pos_a.y + bullet_data->len * -bullet->dir.y;
		game->laser_bro->draw_misc(game->ren, game->laser_bro, &draw_data);

		if (game->game_debug)
			game_render_bullet_debug(game, bullet);

		bullet = bullet->next;
	});
	bro_draw_batch(game->ren, game->laser_bro);
}

static void
game_render_player_server_pos(client_game_t* game, cg_player_t* cg_player)
{
	player_t* player = cg_player->user_data;

	player->rect.pos = cg_player->server_pos;
	player->rect.color.w = 0.5;

	game_render_player_body(game->ren, player);
	player->rect.color.w = 1.0;
}

static void
game_render_players(client_game_t* game)
{
	ren_bind_bro(game->ren, game->ren->default_bro);
	const ght_t* players = &game->cg.players;
	GHT_FOREACH(cg_player_t* cg_player, players, {
		player_t* player = cg_player->user_data;
		if (cg_player->dir.x || cg_player->dir.y)
			player->rect.rotation = atan2(cg_player->dir.y, cg_player->dir.x) + M_PI / 2;

		const vec2f_t origin = rect_origin(&player->rect);
		player->gun_rect.rotation = angle(&origin, &player->core->cursor);

		if (cg_player->gun)
			player->gun_rect.texture = game->gun_textures[cg_player->gun->spec->id];
		else
			player->gun_rect.texture = NULL;

		if (game->game_debug)
		{
			for (u32 i = 0; i < cg_player->cells.count; i++)
			{
				const cg_runtime_cell_t* cell = *(cg_runtime_cell_t**)array_idx(&cg_player->cells, i);
				printf("cell: %u/%u\n", cell->pos.x, cell->pos.y);
				u32 color = 0xFFFFFFAA;
				if (cell->type == CG_CELL_BLOCK)
					color = 0xFF0000AA;

				u32 grid_size = game->cg.map->grid_size;

				rect_t r;
				rect_init(&r, 
					vec2f(
						cell->pos.x * grid_size, 
						cell->pos.y * grid_size
					), 
					vec2f(grid_size, grid_size), color, NULL
				);
				ren_draw_rect(game->ren, &r);
			}
		}

		if (game->game_netdebug)
			game_render_player_server_pos(game, cg_player);

		game_render_player(game->ren, player);
	});
}

UNUSED static void 
game_render_cell_debug(ren_t* ren, rect_t* cell_rect, const cg_runtime_cell_t* cell)
{
	if (cell->type != CG_CELL_BLOCK)
	{
		const cg_empty_cell_data_t* data = cell->data;
		vec4f_t new_color = rgba(0xFFFFFF00);
		for (u32 i = 0; i < data->contents.count; i++)
			new_color.w += 0.3;

		cell_rect->texture = NULL;
		cell_rect->color = new_color;

		ren_draw_rect(ren, cell_rect);
	}
}

static void
game_render_cell(waapp_t* app, const cg_runtime_map_t* map, const cg_runtime_cell_t* cell)
{
	const u32 grid_size = map->grid_size;
	ren_t* ren = &app->ren;
	texture_t* texture = NULL;
	rect_t cell_rect = {0};
	u32 color = 0xFFFFFFFF;

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

	// if (app->game && app->game->game_debug)
	// 	game_render_cell_debug(ren, &cell_rect, cell);
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

// static void
// game_render_map_cell_edges(ren_t* ren, cg_runtime_map_t* map)
// {
// 	const cg_line_t* edges = (const cg_line_t*)map->runtime.edge_pool.buf;
//
// 	for (u32 i = 0; i < map->runtime.edge_pool.count; i++)
// 	{
// 		const cg_line_t* line = edges + i;
// 		ren_draw_line(ren, &line->a, &line->b, 0xFFFFFFFF);
//
// 		rect_t r = {0};
// 		rect_init(&r, line->a, vec2f(16, 16), 0xFF0000FF, NULL);
// 		r.pos.x -= r.size.x;
// 		r.pos.y -= r.size.y;
//
// 		ren_draw_rect(ren, &r);
//
// 		rect_init(&r, line->b, vec2f(16, 16), 0x0000FFFF, NULL);
//
// 		ren_draw_rect(ren, &r);
// 	}
// }

void
game_render_map(waapp_t* app, cg_runtime_map_t* map, bool show_grid)
{
	ren_t* ren = &app->ren;
	const vec2f_t cam = vec2f(-app->cam.x / ren->scale.x, -app->cam.y / ren->scale.y);
	const vec2f_t* viewport = &app->ren.viewport;
	const vec2f_t bot_right = vec2f(cam.x + viewport->x / ren->scale.x, cam.y + viewport->y / ren->scale.y);
	const cg_runtime_cell_t* c_left;
	const cg_runtime_cell_t* c_right;

	c_left = cg_map_at_wpos_clamp(map, &cam);
	c_right = cg_map_at_wpos_clamp(map, &bot_right);

	for (u32 x = c_left->pos.x; x <= c_right->pos.x; x++)
		for (u32 y = c_left->pos.y; y <= c_right->pos.y; y++)
			game_render_cell(app, map, cg_runtime_map_at(map, x, y));


	if (show_grid)
	{
		game_render_grid(ren, map->w, map->h, map->grid_size, map->grid_size);
		// game_render_map_cell_edges(ren, map);
	}
	ren->line_bro->draw_rect(ren, ren->line_bro, &app->map_border);

	ren_bind_bro(ren, app->ren.default_bro);
}

static void 
game_render_screen_ui(client_game_t* game, ren_t* ren)
{
	game_render_progress_bar(ren, ren->screen_bro, &game->health_bar);
	game_render_progress_bar(ren, ren->screen_bro, &game->guncharge_bar);
	bro_draw_batch(ren, ren->screen_bro);
}

void 
game_draw(client_game_t* game)
{
	ren_t* ren = game->ren;
	waapp_t* app = game->app;

	if (app->headless)
		return;

	ren_bind_bro(ren, ren->default_bro);

	game_move_cam(app);

	game_render_map(app, game->cg.map, false);
	ren_draw_batch(ren);

	game_render_bullets(game);
	game_render_players(game);

    ren_draw_batch(ren);

	bro_draw_batch(ren, ren->line_bro);

	game_render_screen_ui(game, ren);
}

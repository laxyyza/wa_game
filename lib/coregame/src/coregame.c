#include "coregame.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef __linux__
#include <sys/random.h>
#endif // __linux__
#ifdef _WIN32
#include <windows.h>
#include <ntsecapi.h>
#endif // _WIN32

static void
coregame_get_delta_time(coregame_t* coregame)
{
	struct timespec current_time;
	clock_gettime(CLOCK_MONOTONIC, &current_time);

	coregame->delta =	(current_time.tv_sec - coregame->last_time.tv_sec) + 
						(current_time.tv_nsec - coregame->last_time.tv_nsec) / 1e9;

	memcpy(&coregame->last_time, &current_time, sizeof(struct timespec));
}


void 
coregame_init(coregame_t* coregame, bool client)
{
	ght_init(&coregame->players, 10, free);

	coregame->world_border = cg_rect(
		vec2f(0, 0),
		vec2f(10000, 10000)
	);
	coregame_get_delta_time(coregame);

	coregame->interp_factor = INTERPOLATE_FACTOR;
	coregame->interp_threshold_dist = INTERPOLATE_THRESHOLD_DIST;
	coregame->client = client;

	// coregame->map = cg_map_new(100, 100, 100);
	coregame->map = cg_map_load("res/test.cgmap");
	mmframes_init(&coregame->mmf);
}

static inline bool 
rect_aabb_test(const cg_rect_t* a, const cg_rect_t* b)
{
	return	(a->pos.x < b->pos.x + b->size.x) &&
			(a->pos.x + a->size.x > b->pos.x) &&
			(a->pos.y < b->pos.y + b->size.y) && 
			(a->pos.y + a->size.y > b->pos.y);
}

f32 
coregame_dist(const vec2f_t* a, const vec2f_t* b)
{
	return sqrtf(powf(b->x - a->x, 2) + powf(b->y - a->y, 2));
}

static void 
coregame_interpolate_player(coregame_t* coregame, cg_player_t* player)
{
	const vec2f_t* client_pos = &player->pos;
	const vec2f_t* server_pos = &player->server_pos;
	f32 dist = coregame_dist(client_pos, server_pos);

	if (dist > coregame->interp_threshold_dist)
	{
		player->pos.x = client_pos->x + (server_pos->x - client_pos->x) * coregame->interp_factor;
		player->pos.y = client_pos->y + (server_pos->y - client_pos->y) * coregame->interp_factor;
	}
	else
	{
		player->pos = player->server_pos;
		player->interpolate = false;
	}
}

static bool 
rect_collide_cell(cg_map_t* map, const cg_rect_t* rect, const cg_cell_t* cell)
{
	if (cell->type != CG_CELL_BLOCK)
		return false;

	const cg_rect_t cell_rect = {
		.pos.x = cell->pos.x * map->header.grid_size,
		.pos.y = cell->pos.y * map->header.grid_size,
		.size.x = map->header.grid_size,
		.size.y = map->header.grid_size
	};
	return rect_aabb_test(rect, &cell_rect);
}

static bool
rect_collide_map(coregame_t* cg, const cg_rect_t* rect)
{
	cg_map_t* map = cg->map;
	vec2f_t bot_right = vec2f(rect->pos.x + rect->size.x, rect->pos.y + rect->size.y);
	cg_cell_t* c_left = cg_map_at_wpos(map, &rect->pos);
	cg_cell_t* c_right = cg_map_at_wpos(map, &bot_right);
	cg_cell_t* cell;

	if (c_left == NULL || c_right == NULL)
		return true;
	if (c_left == c_right)
		return rect_collide_cell(map, rect, c_left);

	for (i32 x = c_left->pos.x; x <= c_right->pos.x; x++)
	{
		for (i32 y = c_left->pos.y; y <= c_right->pos.y; y++)
		{
			if ((cell = cg_map_at(map, x, y)) == NULL)
				return true;
			if (rect_collide_cell(map, rect, cell))
				return true;
		}
	}
	return false;
}

static void 
coregame_move_player(coregame_t* coregame, cg_player_t* player)
{
	if (player->dir.x || player->dir.y)
	{
		const cg_rect_t new_rect = cg_rect(
			vec2f(
				player->pos.x + player->dir.x * PLAYER_SPEED * coregame->delta,
				player->pos.y + player->dir.y * PLAYER_SPEED * coregame->delta
			),
			player->size
		);

		if (!rect_collide_map(coregame, &new_rect))
			memcpy(&player->pos, &new_rect, sizeof(vec2f_t));
	}

	if (player->prev_dir.x != player->dir.x || player->prev_dir.y != player->dir.y ||
		player->prev_pos.x != player->pos.x || player->prev_pos.y != player->pos.y)
	{
		if (coregame->player_changed)
			coregame->player_changed(player, coregame->user_data);
		player->prev_pos = player->pos;
		player->prev_dir = player->dir;
	}
}

static void 
coregame_update_players(coregame_t* coregame)
{
	const ght_t* players = &coregame->players;
	GHT_FOREACH(cg_player_t* player, players, {
		if (player->interpolate)
			coregame_interpolate_player(coregame, player);
		else
			coregame_move_player(coregame, player);
	});
}

static bool
coregame_proj_hit_player_test(coregame_t* coregame, cg_projectile_t* proj)
{
	const ght_t* players = &coregame->players;

	GHT_FOREACH(cg_player_t* player, players, {
		if (player->id != proj->owner && player->health > 0)
		{
			cg_rect_t rect = cg_rect(player->pos, player->size);
			if (rect_aabb_test(&proj->rect, &rect))
			{
				if (coregame->player_damaged)
				{
					player->health -= PROJ_DMG;
					if (player->health < 0)
						player->health = 0;
						coregame->player_damaged(player, coregame->user_data);
				}
				return true;
			}
		}
	});
	return false;
}

static void 
coregame_update_projectiles(coregame_t* coregame)
{
	cg_projectile_t* proj = coregame->proj.head;
	cg_projectile_t* proj_next;

	while (proj)
	{
		proj_next = proj->next;
		proj->rect.pos.x += proj->dir.x * PROJ_SPEED * coregame->delta;
		proj->rect.pos.y += proj->dir.y * PROJ_SPEED * coregame->delta;

		if (coregame_proj_hit_player_test(coregame, proj) || 
			rect_collide_map(coregame, &proj->rect))
		{
			coregame_free_projectile(coregame, proj);
		}
		proj = proj_next;
	}
}

void 
coregame_update(coregame_t* cg)
{
	coregame_get_delta_time(cg);

	coregame_update_players(cg);
	coregame_update_projectiles(cg);
	mmframes_clear(&cg->mmf);
}

void 
coregame_cleanup(coregame_t* cg)
{
	ght_destroy(&cg->players);
	mmframes_free(&cg->mmf);
}

cg_player_t* 
coregame_add_player(coregame_t* coregame, const char* name)
{
	cg_player_t* player = calloc(1, sizeof(cg_player_t));
	coregame_randb(&player->id, sizeof(u32));
	strncpy(player->username, name, PLAYER_NAME_MAX);
	player->pos = vec2f(0, 0);
	player->size = vec2f(150, 150);
	player->health = PLAYER_HEALTH;
	player->max_health = PLAYER_HEALTH;

	coregame_add_player_from(coregame, player);

	return player;
}

void 
coregame_add_player_from(coregame_t* coregame, cg_player_t* player)
{
	ght_insert(&coregame->players, player->id, player);
}

void 
coregame_free_player(coregame_t* coregame, cg_player_t* player)
{
	if (coregame->player_free_callback)
		coregame->player_free_callback(player, coregame->user_data);
	ght_del(&coregame->players, player->id);
}

void 
coregame_set_player_dir(cg_player_t* player, u8 dir)
{
	vec2f_t dir_vec = {0.0, 0.0};
	
	if (dir & PLAYER_DIR_UP)
		dir_vec.y -= 1.0;
	if (dir & PLAYER_DIR_DOWN)
		dir_vec.y += 1.0;
	if (dir & PLAYER_DIR_LEFT)
		dir_vec.x -= 1.0;
	if (dir & PLAYER_DIR_RIGHT)
		dir_vec.x += 1.0;

	player->dir = dir_vec;

	vec2f_norm(&player->dir);
}

cg_projectile_t*
coregame_player_shoot(coregame_t* coregame, cg_player_t* player, vec2f_t dir)
{
	cg_projectile_t* proj = coregame_add_projectile(coregame, player);
	proj->dir = dir;
	if (coregame->client)
		proj->rotation = atan2(proj->dir.y, proj->dir.x) + M_PI / 2;
	return proj;
}

cg_projectile_t* 
coregame_add_projectile(coregame_t* coregame, cg_player_t* player)
{
	cg_projectile_t* proj = calloc(1, sizeof(cg_projectile_t));
	proj->owner = player->id;
	proj->rect.pos = player->pos;
	proj->rect.pos.x += player->size.x / 2;
	proj->rect.pos.y += player->size.y / 2;
	proj->rect.size = vec2f(5, 30);

	if (coregame->proj.tail == NULL)
	{
		coregame->proj.tail = coregame->proj.head = proj;
	}
	else
	{
		coregame->proj.tail->next = proj;
		proj->prev = coregame->proj.tail;
		coregame->proj.tail = proj;
	}

	return proj;
}

void 
coregame_free_projectile(coregame_t* coregame, cg_projectile_t* proj)
{
	if (coregame->proj_free_callback)
		coregame->proj_free_callback(proj, coregame->user_data);

	if (proj->prev)
		proj->prev->next = proj->next;
	if (proj->next)
		proj->next->prev = proj->prev;
	if (proj == coregame->proj.head)
		coregame->proj.head = proj->next;
	if (proj == coregame->proj.tail)
		coregame->proj.tail = proj->next;

	free(proj);
}

void 
coregame_randb(void* buf, u64 count)
{
#ifdef __linux__
	getrandom(buf, count, 0);
#endif
#ifdef _WIN32
	RtlGenRandom(buf, count);
#endif
}

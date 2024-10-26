#define _GNU_SOURCE
#include "coregame.h"
#include <stdlib.h>
#include <sys/random.h>
#include <string.h>
#include <math.h>

void
coregame_norm(vec2f_t* vec)
{
	const f32 manitude = sqrtf(vec->x * vec->x + vec->y * vec->y);


	if (manitude != 0)
	{
		vec->x /= manitude;
		vec->y /= manitude;
	}
}

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
coregame_init(coregame_t* coregame)
{
	ght_init(&coregame->players, 10, free);
	ght_init(&coregame->projectiles, 10, free);

	coregame->world_border = cg_rect(
		vec2f(-300, -300),
		vec2f(10000, 10000)
	);
	coregame_get_delta_time(coregame);
}

static inline bool 
rect_aabb_test(const cg_rect_t* a, const cg_rect_t* b)
{
	return	(a->pos.x < b->pos.x + b->size.x) &&
			(a->pos.x + a->size.x > b->pos.x) &&
			(a->pos.y < b->pos.y + b->size.y) && 
			(a->pos.y + a->size.y > b->pos.y);
}

static inline bool
rect_world_border_test(const coregame_t* coregame, const cg_rect_t* rect)
{
	const cg_rect_t* wb = &coregame->world_border;
	return (rect->pos.x < wb->pos.x ||
			rect->pos.x + rect->size.x > wb->pos.x + wb->size.x ||
			rect->pos.y < wb->pos.y ||
			rect->pos.y + rect->size.y > wb->pos.y + wb->size.y);
}

static void 
coregame_update_players(coregame_t* coregame)
{
	const ght_t* players = &coregame->players;
	GHT_FOREACH(cg_player_t* player, players, {
		if (player->dir.x || player->dir.y)
		{
			const cg_rect_t new_rect = cg_rect(
				vec2f(
					player->rect.pos.x + player->dir.x * PLAYER_SPEED * coregame->delta,
					player->rect.pos.y + player->dir.y * PLAYER_SPEED * coregame->delta
				),
				player->rect.size
			);

			if (!rect_world_border_test(coregame, &new_rect))
				memcpy(&player->rect.pos, &new_rect, sizeof(vec2f_t));
		}
	});
}

// static void 
// coregame_update_projectiles(coregame_t* coregame)
// {
//
// }

void 
coregame_update(coregame_t* coregame)
{
	coregame_get_delta_time(coregame);

	coregame_update_players(coregame);
}

void 
coregame_cleanup(coregame_t* coregame)
{
	ght_destroy(&coregame->projectiles);
	ght_destroy(&coregame->players);
}

cg_player_t* 
coregame_add_player(coregame_t* coregame, const char* name)
{
	cg_player_t* player = calloc(1, sizeof(cg_player_t));
	getrandom(&player->id, sizeof(u32), 0);
	strncpy(player->username, name, PLAYER_NAME_MAX);
	player->rect = cg_rect(
		vec2f(50, 50),		// position
		vec2f(150, 150)		// size
	);
	player->health = PLAYER_HEALTH;

	ght_insert(&coregame->players, player->id, player);

	return player;
}

void 
coregame_free_player(coregame_t* coregame, cg_player_t* player)
{
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

	coregame_norm(&player->dir);
}

cg_projectile_t* 
coregame_add_projectile(coregame_t* coregame, cg_player_t* player)
{
	cg_projectile_t* proj = calloc(1, sizeof(cg_projectile_t));
	getrandom(&proj->id, sizeof(u32), 0);
	proj->owner = player->id;
	proj->rect.pos = player->rect.pos;
	proj->rect.size = vec2f(10, 20);

	ght_insert(&coregame->projectiles, proj->id, proj);

	return proj;
}

void 
coregame_free_projectile(coregame_t* coregame, cg_projectile_t* proj)
{
	ght_del(&coregame->projectiles, proj->id);
}

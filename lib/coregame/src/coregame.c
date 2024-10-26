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

void 
coregame_init(coregame_t* coregame)
{
	ght_init(&coregame->players, 10, free);
	ght_init(&coregame->projectiles, 10, free);

	coregame->world_border = cg_rect(
		vec2f(-100, -100),
		vec2f(1000, 1000)
	);
}

static void 
coregame_update_players(coregame_t* coregame)
{
	const ght_t* players = &coregame->players;
	GHT_FOREACH(cg_player_t* player, players, {
		if (player->dir.x || player->dir.y)
		{
			player->rect.pos.x += player->dir.x * PLAYER_SPEED;
			player->rect.pos.y += player->dir.y * PLAYER_SPEED;
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
		vec2f(100, 100)		// size
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

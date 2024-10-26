#include "coregame.h"
#include <stdlib.h>
#include <sys/random.h>
#include <string.h>

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

void 
coregame_update(UNUSED coregame_t* coregame)
{
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

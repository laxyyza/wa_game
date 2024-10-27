#include "player.h"
#include "app.h"

player_t* 
player_new_from(waapp_t* app, cg_player_t* cg_player)
{
	player_t* player = calloc(1, sizeof(player_t));

	player->core = cg_player;
	rect_init(&player->rect, player->core->pos, 
			player->core->size, 0x000000FF, 
			app->tank_bottom_tex);
	memcpy(&player->top, &player->rect, sizeof(rect_t));
	player->top.texture = app->tank_top_tex;
	ght_insert(&app->players, player->core->id, player);

	return player;
}

player_t* 
player_new(waapp_t* app, const char* name)
{
	cg_player_t* cg_player = coregame_add_player(&app->game, name);

	return player_new_from(app, cg_player);
}

projectile_t* 
projectile_new(waapp_t* app, cg_projectile_t* core_proj)
{
	projectile_t* proj = calloc(1, sizeof(projectile_t));
	proj->core = core_proj;
	rect_init(&proj->rect, proj->core->rect.pos, proj->core->rect.size, 0xFF0000FF, NULL);
	proj->rect.rotation = atan2(proj->core->dir.y, proj->core->dir.x) + M_PI / 2;
	ght_insert(&app->projectiles, proj->core->id, proj);
	return proj;
}

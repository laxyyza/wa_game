#include "player.h"
#include "app.h"


player_t* 
player_new(waapp_t* app, const char* name)
{
	player_t* player = calloc(1, sizeof(player_t));

	player->core = coregame_add_player(&app->game, name);
	rect_init(&player->rect, player->core->rect.pos, 
			player->core->rect.size, 0x000000FF, 
			app->tank_bottom_tex);
	memcpy(&player->top, &player->rect, sizeof(rect_t));
	player->top.texture = app->tank_top_tex;

	return player;
}

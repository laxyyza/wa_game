#include "player.h"
#include "app.h"


player_t* 
player_new(waapp_t* app, const char* name)
{
	player_t* player = calloc(1, sizeof(player_t));

	player->core = coregame_add_player(&app->game, name);
	rect_init(&player->rect, player->core->rect.pos, player->core->rect.size, 0xFF00FFFF, NULL);

	return player;
}

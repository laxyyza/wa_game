#include "player.h"
#include "game.h"

player_t* 
player_new_from(client_game_t* game, cg_player_t* cg_player)
{
	player_t* player = calloc(1, sizeof(player_t));
	vec2f_t gun_size = vec2f(250, 250);

	player->core = cg_player;
	rect_init(&player->rect, player->core->pos, 
			player->core->size, 0x000000FF, 
			game->tank_bottom_tex);
	rect_init(&player->gun_rect, player->rect.pos, gun_size, 0, NULL);

	progress_bar_init(&player->hpbar, player->rect.pos, vec2f(150, 15), 
				   &cg_player->max_health, &cg_player->health, 
				   0xFF0000CC);

	player->hpbar.parent_pos = &cg_player->pos;
	player->hpbar.offset = vec2f(
		(player->rect.size.x - player->hpbar.background.size.x) / 2,
		-30
	);

	progress_bar_init(&player->guncharge, player->rect.pos, vec2f(150, 15), 
				   NULL, NULL, 0x1111FFCC);

	player->guncharge.parent_pos = &cg_player->pos;	
	player->guncharge.offset = vec2f(
		player->hpbar.offset.x,
		player->hpbar.offset.y - (player->hpbar.background.size.y + 3)
	);

	cg_player->user_data = player;

	return player;
}

player_t* 
player_new(client_game_t* game, const char* name)
{
	cg_player_t* cg_player = coregame_add_player(&game->cg, name);

	return player_new_from(game, cg_player);
}

void 
player_update_guncharge(player_t* player, progress_bar_t* bar)
{
	const cg_gun_t* gun = player->core->gun;
	if (gun == NULL)
		return;
	progress_bar_t* guncharge;
	if (bar)
		guncharge = bar;
	else
	{
		guncharge = &player->guncharge;
		progress_bar_update_pos(guncharge);
	}

	if (gun->spec->initial_charge_time)
		progress_bar_update_valmax(guncharge, gun->charge_time, gun->spec->initial_charge_time);
	else
		progress_bar_update_valmax(guncharge, gun->bullet_timer, gun->spec->bullet_spawn_interval);
}

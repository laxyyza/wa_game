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

	rect_init(&player->hpbar.background, player->rect.pos, vec2f(150, 15), 0x000000AA, NULL);

	rect_init(
		&player->hpbar.fill, 
		player->hpbar.background.pos, 
		player->hpbar.background.size,
		0xFF0000CC, 
		NULL
	);
	player->hpbar.fill_width = player->hpbar.fill.size.x;

	player_set_health(player, player->core->health);

	rect_init(&player->guncharge.background, player->rect.pos, vec2f(150, 15), 0x000000AA, NULL);
	rect_init(
		&player->guncharge.fill, 
		vec2f(
			player->guncharge.background.pos.x,
			player->guncharge.background.pos.y - player->guncharge.background.size.y + 3
		), 
		player->guncharge.background.size,
		0x1111FFCC, 
		NULL
	);
	player->guncharge.fill_width = player->guncharge.fill.size.x;

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
player_set_health(player_t* player, f32 new_hp)
{
	if (new_hp > player->core->max_health)
		new_hp = player->core->max_health;
	player->core->health = new_hp;

	f32 hp_per = ((f32)new_hp / (f32)player->core->max_health) * 100.0;

	f32 hpbar_fill = (hp_per * player->hpbar.fill_width) / 100.0;
	player->hpbar.fill.size.x = hpbar_fill;
}

void 
player_update_guncharge(player_t* player)
{
	const cg_gun_t* gun = player->core->gun;
	if (gun == NULL)
		return;

	f32 hp_per;
	if (gun->spec->initial_charge_time)
		hp_per = ((f32)gun->charge_time / (f32)gun->spec->initial_charge_time) * 100.0;
	else
		hp_per = ((f32)gun->bullet_timer / (f32)gun->spec->bullet_spawn_interval) * 100.0;

	f32 hpbar_fill = (hp_per * player->guncharge.fill_width) / 100.0;
	player->guncharge.fill.size.x = hpbar_fill;
}

#include "coregame.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "cutils.h"

#ifdef __linux__
#include <sys/random.h>
#endif // __linux__
#ifdef _WIN32
#include <windows.h>
#include <ntsecapi.h>
#endif // _WIN32

static void
coregame_get_delta_time(coregame_t* cg)
{
	struct timespec current_time;
	clock_gettime(CLOCK_MONOTONIC, &current_time);

	cg->delta =	((current_time.tv_sec - cg->last_time.tv_sec) + 
				(current_time.tv_nsec - cg->last_time.tv_nsec) / 1e9) * cg->time_scale;

	memcpy(&cg->last_time, &current_time, sizeof(struct timespec));
}

static cg_bullet_t*
cg_add_bullet(coregame_t* cg, cg_gun_t* gun)
{
	cg_bullet_t* bullet = calloc(1, sizeof(cg_bullet_t));
	const cg_player_t* player = gun->owner;

	bullet->owner = player->id;
	bullet->r.pos = player->pos;
	bullet->r.size = vec2f(5, 5);
	bullet->r.pos.x += (player->size.x / 2) + (player->dir.x * PLAYER_SPEED * cg->delta);
	bullet->r.pos.y += (player->size.y / 2) + (player->dir.y * PLAYER_SPEED * cg->delta);
	bullet->dmg = gun->spec->dmg;

	bullet->dir.x = player->cursor.x - bullet->r.pos.x;
	bullet->dir.y = player->cursor.y - bullet->r.pos.y;
	vec2f_norm(&bullet->dir);

	if (cg->bullets.tail == NULL)
	{
		cg->bullets.tail = cg->bullets.head = bullet;
	}
	else
	{
		cg->bullets.tail->next = bullet;
		bullet->prev = cg->bullets.tail;
		cg->bullets.tail = bullet;
	}

	return bullet;
}

static void
cg_default_gun_shoot(coregame_t* cg, cg_gun_t* gun)
{
	cg_player_t* player = gun->owner;
	cg_bullet_t* bullet = cg_add_bullet(cg, gun);

	bullet->velocity.x = bullet->dir.x * gun->spec->bullet_speed;
	bullet->velocity.y = bullet->dir.y * gun->spec->bullet_speed;

	if (gun->spec->knockback_force)
	{
		// TODO: Actually implement knockback force.
		player->pos.x += (bullet->dir.x * -1.0) * gun->spec->knockback_force * cg->delta;
		player->pos.y += (bullet->dir.y * -1.0) * gun->spec->knockback_force * cg->delta;
	}
}

void 
coregame_init(coregame_t* coregame, bool client, cg_map_t* map)
{
	ght_init(&coregame->players, 10, free);
	coregame->time_scale = 1.0;

	coregame->world_border = cg_rect(
		vec2f(0, 0),
		vec2f(10000, 10000)
	);
	coregame_get_delta_time(coregame);

	coregame->interp_factor = INTERPOLATE_FACTOR;
	coregame->interp_threshold_dist = INTERPOLATE_THRESHOLD_DIST;
	coregame->client = client;

	// coregame->map = cg_map_new(100, 100, 100);
	if (map)
		coregame->map = map;
	// else
	// 	coregame->map = cg_map_load(MAP_PATH "/test.cgmap");
	
	array_init(&coregame->gun_specs, sizeof(cg_gun_spec_t), 4);
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
		if (player->gun)
			coregame_gun_update(coregame, player->gun);

		if (player->interpolate)
			coregame_interpolate_player(coregame, player);
		else
			coregame_move_player(coregame, player);
	});
}

static bool
coregame_bullet_hit_player_test(coregame_t* coregame, cg_bullet_t* bullet)
{
	ght_t* players = &coregame->players;
	cg_player_t* attacker_player;

	GHT_FOREACH(cg_player_t* target_player, players, {
		if (target_player->id != bullet->owner && target_player->health > 0)
		{
			cg_rect_t rect = cg_rect(target_player->pos, target_player->size);
			if (rect_aabb_test(&bullet->r, &rect))
			{
				if (coregame->player_damaged)
				{
					attacker_player = ght_get(players, bullet->owner);
					if (attacker_player == NULL)
						return true;

					target_player->health -= bullet->dmg;
					if (target_player->health < 0)
						target_player->health = 0;

					coregame->player_damaged(target_player, attacker_player, coregame->user_data);
				}
				return true;
			}
		}
	});
	return false;
}

static void
coregame_update_bullets(coregame_t* cg)
{
	cg_bullet_t* bullet = cg->bullets.head;
	cg_bullet_t* bullet_next;

	while (bullet)
	{
		bullet_next = bullet->next;

		bullet->r.pos.x += bullet->velocity.x * cg->delta;
		bullet->r.pos.y += bullet->velocity.y * cg->delta;

		if (coregame_bullet_hit_player_test(cg, bullet) || 
			rect_collide_map(cg, &bullet->r))
		{
			coregame_free_bullet(cg, bullet);
		}

		bullet = bullet_next;
	}
}

void 
coregame_update(coregame_t* cg)
{
	coregame_get_delta_time(cg);

	coregame_update_players(cg);
	// coregame_update_projectiles(cg);
	coregame_update_bullets(cg);
}

void 
coregame_cleanup(coregame_t* cg)
{
	free(cg->map);
	array_del(&cg->gun_specs);
	ght_destroy(&cg->players);
}

cg_player_t* 
coregame_add_player(coregame_t* coregame, const char* name)
{
	cg_player_t* player = calloc(1, sizeof(cg_player_t));
	coregame_randb(&player->id, sizeof(u32));
	strncpy(player->username, name, PLAYER_NAME_MAX - 1);
	player->pos = vec2f(0, 0);
	player->size = vec2f(150, 150);
	player->health = PLAYER_HEALTH;
	player->max_health = PLAYER_HEALTH;

	coregame_add_player_from(coregame, player);

	return player;
}

void 
coregame_add_player_from(coregame_t* cg, cg_player_t* player)
{
	ght_insert(&cg->players, player->id, player);
}

void 
coregame_free_player(coregame_t* coregame, cg_player_t* player)
{
	if (coregame->player_free_callback)
		coregame->player_free_callback(player, coregame->user_data);
	free(player->gun);
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

void 
coregame_free_bullet(coregame_t* coregame, cg_bullet_t* bullet)
{
	if (coregame->bullet_free_callback)
		coregame->bullet_free_callback(bullet, coregame->user_data);

	if (bullet->prev)
		bullet->prev->next = bullet->next;
	if (bullet->next)
		bullet->next->prev = bullet->prev;
	if (bullet == coregame->bullets.head)
		coregame->bullets.head = bullet->next;
	if (bullet == coregame->bullets.tail)
		coregame->bullets.tail = bullet->next;

	free(bullet);
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

void
coregame_gun_update(coregame_t* cg, cg_gun_t* gun)
{
	if (gun->owner->shoot)
		gun->bullet_timer += cg->delta;
	else if (gun->bullet_timer > 0)
	{
		gun->bullet_timer -= cg->delta;
		if (gun->bullet_timer < 0)
			gun->bullet_timer = 0;
	}

	while (gun->bullet_timer >= gun->spec->bullet_spawn_interval)
	{
		cg_default_gun_shoot(cg, gun);
		gun->bullet_timer -= gun->spec->bullet_spawn_interval;
	}
}

static const cg_gun_spec_t* 
cg_get_gun_spec(const coregame_t* cg, enum cg_gun_id id)
{
	const cg_gun_spec_t* specs = (const cg_gun_spec_t*)cg->gun_specs.buf;

	for (u32 i = 0; i < cg->gun_specs.count; i++)
		if (specs[i].id == id)
			return specs + i;
	return NULL;
}

cg_gun_t* 
coregame_create_gun(coregame_t* cg, enum cg_gun_id id, cg_player_t* owner)
{
	const cg_gun_spec_t* spec;
	cg_gun_t* gun;

	if ((spec = cg_get_gun_spec(cg, id)) == NULL)
		return NULL;

	gun = calloc(1, sizeof(cg_gun_t));
	gun->spec = spec;
	gun->owner = owner;
	owner->gun = gun;

	return gun;
}

void 
coregame_add_gun_spec(coregame_t* cg, const cg_gun_spec_t* spec)
{
	cg_gun_spec_t* new_spec = array_add_into(&cg->gun_specs);

	memcpy(new_spec, spec, sizeof(cg_gun_spec_t));
	new_spec->bullet_spawn_interval = 1.0 / spec->bps;
}

bool
coregame_player_change_gun(coregame_t* cg, cg_player_t* player, enum cg_gun_id id)
{
	if (player->gun)
	{
		if (player->gun->spec->id == id)
			return false;

		free(player->gun);
	}

	return coregame_create_gun(cg, id, player) != NULL;
}

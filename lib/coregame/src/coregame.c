#include "coregame.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "cutils.h"

#ifdef _WIN32
#define isnanf(x) _isnanf(x)
#endif // _WIN32

static inline void
cg_swapf32(f32* a, f32* b)
{
	const f32 tmp = *a;
	*a = *b;
	*b = tmp;
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

static void
cg_remove_player_from_cell(cg_player_t* player, cg_runtime_cell_t* cell)
{
	cg_empty_cell_data_t* data = cell->data;
	for (u32 i = 0; i < data->contents.count; i++)
	{
		cg_player_t* player_in_cell = ((cg_player_t**)data->contents.buf)[i];
		if (player_in_cell == player)
		{
			array_erase(&data->contents, i);
			return;
		}
	}
}

static void
cg_player_remove_self_from_cells(cg_player_t* player)
{
	for (u32 i = 0; i < player->cells.count; i++)
	{
		cg_runtime_cell_t* cell = ((cg_runtime_cell_t**)player->cells.buf)[i];
		if (cell->type != CG_CELL_BLOCK)
			cg_remove_player_from_cell(player, cell);
	}
}

static void
cg_player_get_cells(cg_runtime_map_t* map, cg_player_t* player)
{
	array_clear(&player->cells, false);

	vec2f_t pos = vec2f(player->pos.x + player->velocity.x, player->pos.y + player->velocity.y);
	vec2f_t bot_right = vec2f(pos.x + player->size.x, 
							 pos.y + player->size.y);
	cg_runtime_cell_t* c_left = cg_map_at_wpos(map, &pos);
	cg_runtime_cell_t* c_right = cg_map_at_wpos(map, &bot_right);
	cg_runtime_cell_t* cell;

	if (c_left == NULL || c_right == NULL)
	{
		// Probably means the player is out of the map. 
		// TODO: Get inside map if outside.
		return;
	}

	for (i32 x = c_left->pos.x; x <= c_right->pos.x; x++)
	{
		for (i32 y = c_left->pos.y; y <= c_right->pos.y; y++)
		{
			if ((cell = cg_runtime_map_at(map, x, y)) == NULL)
				continue;
			array_add_voidp(&player->cells, cell);
		}
	}
}

static void
cg_player_add_into_cells(cg_player_t* player)
{
	for (u32 i = 0; i < player->cells.count; i++)
	{
		cg_runtime_cell_t* cell = ((cg_runtime_cell_t**)player->cells.buf)[i];
		if (cell->type != CG_CELL_BLOCK)
		{
			cg_empty_cell_data_t* data = cell->data;
			array_add_voidp(&data->contents, player);
		}
	}
}

static bool
cg_ray_collision_test(const vec2f_t* ray_point,
					  const vec2f_t* dir,
					  const cg_rect_t* target,
					  vec2f_t* contact_point,
					  vec2f_t* contact_normal,
					  f32* t_hit_near)
{
	vec2f_t t_near = {
		.x = (target->pos.x - ray_point->x) / dir->x,
		.y = (target->pos.y - ray_point->y) / dir->y
	};
	vec2f_t t_far = {
		.x = (target->pos.x + target->size.x - ray_point->x) / dir->x,
		.y = (target->pos.y + target->size.y - ray_point->y) / dir->y
	};

	if (isnanf(t_far.x) || isnanf(t_far.y)) 
		return false;
	if (isnanf(t_near.x) || isnanf(t_near.y)) 
		return false;

	if (t_near.x > t_far.x)
		cg_swapf32(&t_near.x, &t_far.x);
	if (t_near.y > t_far.y)
		cg_swapf32(&t_near.y, &t_far.y);

	if (t_near.x > t_far.y || t_near.y > t_far.x)
		return false;

	*t_hit_near = fmaxf(t_near.x, t_near.y);
	f32 t_hit_far = fminf(t_far.x, t_far.y);

	if (t_hit_far < 0)
		return false;

	contact_point->x = ray_point->x + *t_hit_near * dir->x;
	contact_point->y = ray_point->y + *t_hit_near * dir->y;

	if (t_near.x > t_near.y)
	{
		if (dir->x < 0)
		{
			contact_normal->x = 1;
			contact_normal->y = 0;
		}
		else
		{
			contact_normal->x = -1;
			contact_normal->y = 0;
		}
	}
	else if (t_near.x < t_near.y)
	{
		if (dir->y < 0)
		{
			contact_normal->x = 0;
			contact_normal->y = 1;
		}
		else
		{
			contact_normal->x = 0;
			contact_normal->y = -1;
		}
	}

	return true;
}

static bool
cg_bullet_cell_collision(const cg_bullet_t* bullet, 
						 const cg_rect_t* target, 
						 vec2f_t* contact_point,
						 vec2f_t* contact_normal,
						 f32* contact_time)
{
	const f32 w = bullet->r.size.x;
	const f32 h = bullet->r.size.y;

	const cg_rect_t expanded_target = {
		.pos.x = target->pos.x - (w / 2.0),
		.pos.y = target->pos.y - (h / 2.0),
		.size.x = target->size.x + w,
		.size.y = target->size.y + h
	};
	const vec2f_t mid = {
		.x = bullet->r.pos.x + (w / 2.0),
		.y = bullet->r.pos.y + (h / 2.0),
	};

	if (cg_ray_collision_test(&mid, 
							&bullet->velocity, 
							&expanded_target, 
							contact_point, 
							contact_normal, 
							contact_time))
	{
		*contact_time = fabsf(*contact_time);
		return (*contact_time < 1.0f);
	}
	return false;
}

static bool
cg_player_cell_collision(const cg_player_t* player, 
						 const cg_rect_t* target, 
						 vec2f_t* contact_point,
						 vec2f_t* contact_normal,
						 f32* contact_time)
{
	const f32 w = player->size.x;
	const f32 h = player->size.y;

	const cg_rect_t expanded_target = {
		.pos.x = target->pos.x - (w / 2.0),
		.pos.y = target->pos.y - (h / 2.0),
		.size.x = target->size.x + w,
		.size.y = target->size.y + h
	};
	const vec2f_t mid = {
		.x = player->pos.x + (w / 2.0),
		.y = player->pos.y + (h / 2.0),
	};

	if (cg_ray_collision_test(&mid, 
							&player->velocity, 
							&expanded_target, 
							contact_point, 
							contact_normal, 
							contact_time))
	{
		return (*contact_time >= 0.0f && *contact_time < 1.0f);
	}
	return false;
}

static void
cg_get_cells_2points(cg_runtime_map_t* map, 
					array_t* cells,
					const vec2f_t* start,
					const vec2f_t* end)
{
	const f32 grid_size = map->grid_size;

	i32 x0 = floorf(start->x / grid_size);
	i32 y0 = floorf(start->y / grid_size);

	i32 x1 = floorf(end->x / grid_size);
	i32 y1 = floorf(end->y / grid_size);

	f32 dx = end->x - start->x;
	f32 dy = end->y - start->y;

	i32 step_x = (dx > 0) ? 1 : -1;
	i32 step_y = (dy > 0) ? 1 : -1;

	f32 t_max_x = ((x0 + (step_x > 0)) * grid_size - start->x) / dx;
	f32 t_max_y = ((y0 + (step_y > 0)) * grid_size - start->y) / dy;

	f32 t_delta_x = fabs(grid_size / dx);
	f32 t_delta_y = fabs(grid_size / dy);

	while (1)
	{
		cg_runtime_cell_t* cell = cg_runtime_map_at(map, x0, y0);
		if (cell)
			array_add_voidp(cells, cell);
		else
			break;

		if (x0 == x1 && y0 == y1)
			break;

		if (t_max_x < t_max_y)
		{
			t_max_x += t_delta_x;
			x0 += step_x;
		}
		else
		{
			t_max_y += t_delta_y;
			y0 += step_y;
		}
	}
}

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
	bullet->r.size = vec2f(10, 10);
	bullet->r.pos.x += (player->size.x / 2) + (player->dir.x * PLAYER_SPEED * cg->delta);
	bullet->r.pos.y += (player->size.y / 2) + (player->dir.y * PLAYER_SPEED * cg->delta);
	bullet->dmg = gun->spec->dmg;
	bullet->gun_id = gun->spec->id;
	array_init(&bullet->cells, sizeof(const cg_runtime_cell_t**), 6);

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

	if (cg->on_bullet_create)
		cg->on_bullet_create(bullet, cg->user_data);

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
		player->velocity.x += -bullet->dir.x * gun->spec->knockback_force;
		player->velocity.y += -bullet->dir.y * gun->spec->knockback_force;
	}
}

void 
coregame_init(coregame_t* coregame, bool client, cg_runtime_map_t* map)
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

	player->velocity.x = player->velocity.y = 0;

	cg_player_remove_self_from_cells(player);
	cg_player_get_cells(coregame->map, player);
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
	cg_player_add_into_cells(player);
}

static void
cg_resolve_player_collision(cg_player_t* player, 
							const vec2f_t* contact_normal, 
							f32 contact_time)
{
	const f32 x_velabs = fabsf(player->velocity.x);
	const f32 y_velabs = fabsf(player->velocity.y);

	const f32 resolved_x = clampf((contact_normal->x * x_velabs) * (1 - contact_time), -x_velabs, x_velabs);
	const f32 resolved_y = clampf((contact_normal->y * y_velabs) * (1 - contact_time), -y_velabs, y_velabs);

	player->velocity.x += resolved_x;
	player->velocity.y += resolved_y;
}

static void
cg_player_handle_block_collision(coregame_t* cg, 
								 cg_player_t* player, 
								 const cg_runtime_cell_t* cell)
{
	const u32 grid_size = cg->map->grid_size;
	vec2f_t contact_normal;
	vec2f_t contact_point;
	f32 contact_time = 0;
	cg_rect_t target = {
		.pos = vec2f(
			cell->pos.x * grid_size, 
			cell->pos.y * grid_size
		),
		.size = vec2f(grid_size, grid_size)
	};

	if (cg_player_cell_collision(player, 
							  &target, 
							  &contact_point, 
							  &contact_normal, 
							  &contact_time))
	{
		cg_resolve_player_collision(player, &contact_normal, contact_time);
	}
}

static void
cg_player_handle_player_collision(cg_player_t* player, const cg_runtime_cell_t* cell)
{
	vec2f_t contact_normal = {0, 0};
	vec2f_t contact_point;
	f32 contact_time = 0;
	const cg_empty_cell_data_t* data = cell->data;

	for (u32 i = 0; i < data->contents.count; i++)
	{
		const cg_player_t* target_player = ((const cg_player_t**)data->contents.buf)[i];

		cg_rect_t target = {
			.pos = target_player->pos,
			.size = target_player->size
		};

		if (cg_player_cell_collision(player, 
								  &target, 
								  &contact_point, 
								  &contact_normal, 
								  &contact_time))
		{
			cg_resolve_player_collision(player, &contact_normal, contact_time);
		}
	}
}

static void
cg_player_handle_collision(coregame_t* cg, cg_player_t* player)
{
	for (u32 i = 0; i < player->cells.count; i++)
	{
		const cg_runtime_cell_t* cell = ((const cg_runtime_cell_t**)player->cells.buf)[i];

		if (cell->type == CG_CELL_BLOCK)
			cg_player_handle_block_collision(cg, player, cell);
		else
			cg_player_handle_player_collision(player, cell);
	}
}

static void 
coregame_move_player(coregame_t* coregame, cg_player_t* player)
{
	if (player->velocity.x || player->velocity.y)
	{
		player->velocity.x *= coregame->delta;
		player->velocity.y *= coregame->delta;

		cg_player_remove_self_from_cells(player);
		cg_player_get_cells(coregame->map, player);
		cg_player_handle_collision(coregame, player);

		player->pos.x += player->velocity.x;
		player->pos.y += player->velocity.y;

		cg_player_get_cells(coregame->map, player);
		cg_player_add_into_cells(player);
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

	GHT_FOREACH(cg_player_t* player, players, 
	{
		player->velocity.x = player->dir.x * PLAYER_SPEED;
		player->velocity.y = player->dir.y * PLAYER_SPEED;

		if (player->gun)
			coregame_gun_update(coregame, player->gun);

		if (player->interpolate)
			coregame_interpolate_player(coregame, player);
		else
			coregame_move_player(coregame, player);
	});
}

static bool
cg_bullet_block_cell_collision_test(coregame_t* cg, 
									cg_bullet_t* bullet, 
									const cg_runtime_cell_t* cell, 
									vec2f_t* next_pos)
{
	u32 grid_size = cg->map->grid_size;
	vec2f_t contact_normal;
	vec2f_t contact_point;
	f32 contact_time = 0;

	cg_rect_t target = {
		.pos = vec2f(cell->pos.x * grid_size, cell->pos.y * grid_size),
		.size = vec2f(grid_size, grid_size)
	};
	if (cg_bullet_cell_collision(
								bullet, 
								&target, 
								&contact_point,	
								&contact_normal, 
								&contact_time))
	{
		bullet->collided = true;
		bullet->contact_point = contact_point;
		*next_pos = contact_point;
		return true;
	}
	return false;
}

static bool
cg_bullet_empty_cell_collision_test(coregame_t* cg, 
									cg_bullet_t* bullet, 
									const cg_runtime_cell_t* cell, 
									vec2f_t* next_pos)
{
	vec2f_t contact_normal;
	vec2f_t contact_point;
	f32 contact_time = 0;
	const cg_empty_cell_data_t* data = cell->data;

	for (u32 i = 0; i < data->contents.count; i++)
	{
		cg_player_t* target_player = ((cg_player_t**)data->contents.buf)[i];
		if (target_player->id == bullet->owner)
			continue;

		const cg_rect_t target = {
			.pos = target_player->pos,
			.size = target_player->size
		};
		if (cg_bullet_cell_collision(bullet, 
									&target, 
									&contact_point, 
									&contact_normal, 
									&contact_time))
		{
			cg_player_t* attacker_player;
			if (cg->player_damaged)
			{
				attacker_player = ght_get(&cg->players, bullet->owner);
				if (attacker_player == NULL)
					return true;

				target_player->health -= bullet->dmg;
				if (target_player->health < 0)
					target_player->health = 0;

				cg->player_damaged(target_player, attacker_player, cg->user_data);
			}

			bullet->collided = true;
			bullet->contact_point = contact_point;
			*next_pos = contact_point;
			return true;
		}
	}
	return false;
}

static bool
cg_bullet_collision_test(coregame_t* cg, cg_bullet_t* bullet, vec2f_t* next_pos)
{
	for (u32 i = 0; i < bullet->cells.count; i++)
	{
		const cg_runtime_cell_t* cell = ((const cg_runtime_cell_t**)bullet->cells.buf)[i];

		if (cell->type == CG_CELL_BLOCK)
		{
			if (cg_bullet_block_cell_collision_test(cg, bullet, cell, next_pos))
				return true;
		}
		else
		{
			if (cg_bullet_empty_cell_collision_test(cg, bullet, cell, next_pos))
				return true;
		}
	}
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

		if (bullet->collided)
			coregame_free_bullet(cg, bullet);
		else
		{
			vec2f_t next_pos = vec2f(
				bullet->r.pos.x + bullet->velocity.x * cg->delta,
				bullet->r.pos.y + bullet->velocity.y * cg->delta
			);
			array_clear(&bullet->cells, false);
			cg_get_cells_2points(cg->map, &bullet->cells, &bullet->r.pos, &next_pos);
			if (cg_bullet_collision_test(cg, bullet, &next_pos))
			{
				bullet->collided = true;
			}
			bullet->r.pos = next_pos;

			if (bullet->r.pos.x < 0 || bullet->r.pos.y < 0 ||
				bullet->r.pos.x > cg->map->w * cg->map->grid_size ||
				bullet->r.pos.y > cg->map->h * cg->map->grid_size)
			{
				coregame_free_bullet(cg, bullet);
			}
		}

		bullet = bullet_next;
	}
}

void 
coregame_update(coregame_t* cg)
{
	coregame_get_delta_time(cg);
	if (cg->pause)
		return;

	coregame_update_players(cg);
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
	player->id = ++coregame->player_id_seq;
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
	array_init(&player->cells, sizeof(cg_runtime_cell_t**), 6);
	cg_player_get_cells(cg->map, player);
	cg_player_add_into_cells(player);
}

void 
coregame_free_player(coregame_t* coregame, cg_player_t* player)
{
	if (coregame->player_free_callback)
		coregame->player_free_callback(player, coregame->user_data);
	free(player->gun);
	cg_player_remove_self_from_cells(player);
	array_del(&player->cells);
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
	array_del(&bullet->cells);

	free(bullet);
}

void
coregame_gun_update(coregame_t* cg, cg_gun_t* gun)
{
	if (gun->ammo <= 0)
	{
		gun->reload_time += cg->delta;

		if (gun->reload_time >= gun->spec->reload_time)
		{
			gun->ammo = gun->spec->max_ammo;
			gun->reload_time = 0;
		}
		else
			return;
	}

	if (gun->owner->shoot)
	{
		if (gun->bullet_timer == 0 && gun->spec->initial_charge_time &&
			gun->charge_time < gun->spec->initial_charge_time)
		{
			gun->charge_time += cg->delta;
		}
		else
			gun->bullet_timer += cg->delta;
	}
	else
	{
		if (gun->spec->initial_charge_time)
		{
			gun->charge_time = clampf(gun->charge_time - (cg->delta * 2), 0, gun->spec->initial_charge_time);
			gun->bullet_timer = 0;
		}
		else
		{
			if (gun->spec->autocharge)
				gun->bullet_timer += cg->delta;
			else
				gun->bullet_timer -= cg->delta;
			gun->bullet_timer = clampf(gun->bullet_timer, 0, gun->spec->bullet_spawn_interval);
		}
		return;
	}

	while (gun->bullet_timer >= gun->spec->bullet_spawn_interval && gun->ammo > 0)
	{
		cg_default_gun_shoot(cg, gun);
		gun->bullet_timer -= gun->spec->bullet_spawn_interval;
		gun->ammo--;
	}
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
	gun->ammo = spec->max_ammo;
	owner->gun = gun;

	return gun;
}

void 
coregame_add_gun_spec(coregame_t* cg, const cg_gun_spec_t* spec)
{
	cg_gun_spec_t* new_spec = array_add_into(&cg->gun_specs);

	memcpy(new_spec, spec, sizeof(cg_gun_spec_t));
	new_spec->bullet_spawn_interval = 1.0 / spec->bps;
	if (new_spec->initial_charge_time)
		new_spec->initial_charge_time = 1.0 / new_spec->initial_charge_time;
	else
		new_spec->initial_charge_time = 0;
}

bool
coregame_player_change_gun(coregame_t* cg, cg_player_t* player, enum cg_gun_id id)
{
	if (player->gun)
	{
		if (player->gun->spec->id == id)
			return false;
		if (player->gun->ammo <= 0)
			return false;

		free(player->gun);
	}

	return coregame_create_gun(cg, id, player) != NULL;
}

void 
coregame_player_reload(cg_player_t* player)
{
	if (player->gun == NULL || player->gun->ammo == player->gun->spec->max_ammo)
		return;

	player->gun->ammo = 0;
}


#include "coregame.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "cutils.h"

#define BLEND_RATE 0.01

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
	hr_time_t current_time;
	nano_gettime(&current_time);

	cg->delta = nano_time_diff_s(&cg->last_time, &current_time) * cg->time_scale;

	memcpy(&cg->last_time, &current_time, sizeof(hr_time_t));
}

cg_bullet_t*
cg_add_bullet(coregame_t* cg, cg_gun_t* gun)
{
	cg_bullet_t* bullet = calloc(1, sizeof(cg_bullet_t));
	const cg_player_t* player = gun->owner;

	bullet->id = ++cg->bullet_id_seq;
	bullet->owner_id = player->id;
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

	ght_insert(&cg->bullets, bullet->id, bullet);

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

static void
cg_do_free_player(cg_player_t* player)
{
	if (player->on_player_free)
		player->on_player_free(player);
	free(player->gun);
	cg_player_remove_self_from_cells(player);
	array_del(&player->cells);
	free(player);
}

static void
cg_do_free_bullet(cg_bullet_t* bullet)
{
	array_del(&bullet->cells);
	free(bullet);
}

void 
coregame_init(coregame_t* coregame, cg_runtime_map_t* map)
{
	ght_init(&coregame->players, 10, (ght_free_t)cg_do_free_player);
	ght_init(&coregame->bullets, 100, (ght_free_t)cg_do_free_bullet);
	coregame->time_scale = 1.0;

	coregame->world_border = cg_rect(
		vec2f(0, 0),
		vec2f(10000, 10000)
	);
	coregame_get_delta_time(coregame);

#ifdef CG_CLIENT
	coregame->local_interp_factor = INTERPOLATE_FACTOR;
	coregame->target_local_interp_factor = INTERPOLATE_FACTOR;
	coregame->remote_interp_factor = INTERPOLATE_FACTOR;
	coregame->target_remote_interp_factor = INTERPOLATE_FACTOR;
	coregame->interp_threshold_dist = INTERPOLATE_THRESHOLD_DIST;
#endif // CG_CLIENT

	if (map)
		coregame->map = map;
	
	array_init(&coregame->gun_specs, sizeof(cg_gun_spec_t), 4);
}

#ifdef CG_SERVER
void 
coregame_server_init(coregame_t* cg, cg_runtime_map_t* map, f32 tick_per_sec)
{
	coregame_init(cg, map);

	cg->sbsm = sbsm_create(tick_per_sec / 4, 1000.0 / tick_per_sec);
}
#endif // CG_SERVER

f32 
coregame_dist(const vec2f_t* a, const vec2f_t* b)
{
	return sqrtf(powf(b->x - a->x, 2) + powf(b->y - a->y, 2));
}

#ifdef CG_CLIENT
static inline void
cg_blend_pos(vec2f_t* dst, const vec2f_t* a, const vec2f_t* b, f32 factor)
{
	dst->x = a->x * (1 - factor) + b->x * factor;
	dst->y = a->y * (1 - factor) + b->y * factor;
}

static inline void
cg_interpolate_pos(vec2f_t* a, const vec2f_t* b, f32 factor)
{
	a->x = a->x + (b->x - a->x) * factor;
	a->y = a->y + (b->y - a->y) * factor;
}
#endif

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
	vec2f_t contact_normal = {0, 0};
	vec2f_t contact_point = {0, 0};
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
	vec2f_t contact_point = {0, 0};
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

void 
coregame_update_player(coregame_t* coregame, cg_player_t* player)
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
	#ifdef CG_SERVER
		player->dirty = true;
		coregame->sbsm->dirty = true;
	#endif // CG_SERVER

		if (coregame->player_changed)
			coregame->player_changed(player, coregame->user_data);

		player->prev_pos = player->pos;
		player->prev_dir = player->dir;
	}
}

#ifdef CG_CLIENT
static void 
coregame_interpolate_player(coregame_t* cg, cg_player_t* player)
{
	vec2f_t* client_pos = &player->pos;
	const vec2f_t* server_pos = &player->server_pos;
	const f32 dist = coregame_dist(client_pos, server_pos);

	if (dist > player->size.x * 4)
	{
		player->bad_local_pos = true;
	}
	else if (dist < cg->interp_threshold_dist)
	{
		player->pos = player->server_pos;
		player->interpolate = false;
		player->bad_local_pos = false;
		return;
	}

	vec2f_t new_pos;
	if (player->bad_local_pos)
	{
		cg_player_remove_self_from_cells(player);
		cg_player_get_cells(cg->map, player);
		cg_interpolate_pos(client_pos, server_pos, 0.1);
	}
	else
	{
		const f32 interp = (player == cg->local_player) ? cg->local_interp_factor : cg->remote_interp_factor;
		cg_blend_pos(&new_pos, client_pos, server_pos, interp);

		player->velocity.x = new_pos.x - client_pos->x;
		player->velocity.y = new_pos.y - client_pos->y;

		cg_player_remove_self_from_cells(player);
		cg_player_get_cells(cg->map, player);
		cg_player_handle_collision(cg, player);

		client_pos->x += player->velocity.x;
		client_pos->y += player->velocity.y;

		const f32 new_dist = coregame_dist(client_pos, server_pos);
		if (new_dist >= dist)
			player->bad_local_pos = true;
	}
	cg_player_add_into_cells(player);
}
#endif // CG_CLIENT

static void 
coregame_update_players(coregame_t* cg)
{
	const ght_t* players = &cg->players;

#ifdef CG_CLIENT
	if (cg->target_local_interp_factor != cg->local_interp_factor)
		cg->local_interp_factor = cg->local_interp_factor * (1 - BLEND_RATE) + cg->target_local_interp_factor * BLEND_RATE;
	if (cg->target_remote_interp_factor != cg->remote_interp_factor)
		cg->remote_interp_factor = cg->remote_interp_factor * (1 - BLEND_RATE) + cg->target_remote_interp_factor * BLEND_RATE;
#endif // CG_CLIENT

	GHT_FOREACH(cg_player_t* player, players, 
	{
		player->velocity.x = player->dir.x * PLAYER_SPEED;
		player->velocity.y = player->dir.y * PLAYER_SPEED;

		if (player->gun)
			coregame_gun_update(cg, player->gun);

		coregame_update_player(cg, player);
	#ifdef CG_CLIENT
		if (player->interpolate)
			 coregame_interpolate_player(cg, player);
	#endif // CG_CLIENT
	
	#ifdef CG_SERVER
		if (player->gun_dirty)
		{
			 cg->player_gun_changed(player, cg->user_data);
			 player->gun_dirty = false;
		}

		sbsm_commit_player(cg->sbsm->present, player);
	#endif // CG_SERVER
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
		if (target_player->id == bullet->owner_id)
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
				attacker_player = ght_get(&cg->players, bullet->owner_id);
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

void
coregame_update_bullet(coregame_t* cg, cg_bullet_t* bullet)
{
	if (bullet->collided)
	{
	#ifdef CG_CLIENT
		coregame_free_bullet(cg, bullet);
	#endif // CG_CLIENT
	}
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

	#ifdef CG_SERVER
		sbsm_commit_bullet(cg->sbsm->present, bullet);
	#endif // CG_SERVER
	}
}

static void
coregame_update_bullets(coregame_t* cg)
{
	ght_t* bullets = &cg->bullets;

	GHT_FOREACH(cg_bullet_t* bullet, bullets, 
	{
		coregame_update_bullet(cg, bullet);
	});
}

void 
coregame_update(coregame_t* cg)
{
	coregame_get_delta_time(cg);
	if (cg->pause)
		return;

#ifdef CG_SERVER
	if (cg->sbsm->oldest_change)
		sbsm_rollback(cg);

	sbsm_rotate(cg, cg->sbsm);
#endif // CG_SERVER

	coregame_update_players(cg);
	coregame_update_bullets(cg);

	// if (cg->sbsm->dirty)
	// {
	// 	sbsm_print(cg->sbsm);
	// 	cg->sbsm->dirty = false;
	// }
}

void 
coregame_cleanup(coregame_t* cg)
{
	ght_destroy(&cg->players);
	ght_destroy(&cg->bullets);
	cg_runtime_map_free(cg->map);
	array_del(&cg->gun_specs);
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
	player->on_player_free = cg->player_free_callback;
}

void 
coregame_free_player(coregame_t* cg, cg_player_t* player)
{
#ifdef CG_SERVER
	sbsm_delete_player(cg->sbsm, player);
#endif // CG_SERVER

	ght_del(&cg->players, player->id);
}

// static inline void
// print_input(u8 input)
// {
// 	if (input & PLAYER_INPUT_LEFT)
// 		printf("LEFT ");
// 	if (input & PLAYER_INPUT_RIGHT)
// 		printf("RIGHT ");
// 	if (input & PLAYER_INPUT_UP)
// 		printf("UP ");
// 	if (input & PLAYER_INPUT_DOWN)
// 		printf("DOWN ");
// }

void 
coregame_set_player_input(cg_player_t* player, u8 input)
{
	vec2f_t dir_vec = {0.0, 0.0};
	
	if (input & PLAYER_INPUT_UP)
		dir_vec.y -= 1.0;
	if (input & PLAYER_INPUT_DOWN)
		dir_vec.y += 1.0;
	if (input & PLAYER_INPUT_LEFT)
		dir_vec.x -= 1.0;
	if (input & PLAYER_INPUT_RIGHT)
		dir_vec.x += 1.0;

	player->dir = dir_vec;

	vec2f_norm(&player->dir);

	player->shoot = input & PLAYER_INPUT_SHOOT;
	player->input = input;
}

#ifdef CG_SERVER
void 
coregame_set_player_input_t(coregame_t* cg, cg_player_t* player, u8 input, f64 timestamp)
{
	cg_game_snapshot_t* ss = sbsm_lookup(cg->sbsm, timestamp);

	if (player->last_input_timestamp > ss->timestamp)
		return;
	else
		player->last_input_timestamp = ss->timestamp;

	ss->dirty = true;
	cg_player_snapshot_t* ps = ght_get(&ss->player_states, player->id);
	if (ps == NULL)
	{
		ps = calloc(1, sizeof(cg_player_snapshot_t));
		ps->player_id = player->id;
		sbsm_player_to_snapshot(ps, player);

		/* TODO: Find the oldest changes */
		// ps->prev = NULL;

		ght_insert(&ss->player_states, player->id, ps);
	}
	else if (ps->input == input || ps->dirty_count > 1)
		return;

	ps->dirty_move = ((ps->input & PLAYER_MOVE_INPUT) != (input & PLAYER_MOVE_INPUT));
	ps->dirty_gun = ((ps->input & PLAYER_GUN_INPUT) != (input & PLAYER_GUN_INPUT));
	ps->input = input;
	ps->dirty = true;
	ps->dirty_count++;

	if (cg->sbsm->oldest_change == NULL || ss->timestamp < cg->sbsm->oldest_change->timestamp)
	{
		cg->sbsm->oldest_change = ss;
	}
}
#endif // CG_SERVER

u8
coregame_get_player_input(const cg_player_t* player)
{
	u8 input = 0;

	if (player->dir.x > 0)
		input |= PLAYER_INPUT_RIGHT;
	else if (player->dir.x < 0)
		input |= PLAYER_INPUT_LEFT;
	if (player->dir.y > 0)
		input |= PLAYER_INPUT_DOWN;
	if (player->dir.y < 0)
		input |= PLAYER_INPUT_UP;

	if (player->shoot)
		input |= PLAYER_INPUT_SHOOT;

	return input;
}

void 
coregame_free_bullet(coregame_t* coregame, cg_bullet_t* bullet)
{
	if (coregame->bullet_free_callback)
		coregame->bullet_free_callback(bullet, coregame->user_data);

	ght_del(&coregame->bullets, bullet->id);
}

void
coregame_gun_update(coregame_t* cg, cg_gun_t* gun)
{
	if (gun->ammo <= 0)
	{
		gun->reload_time += cg->delta;

	#ifdef CG_SERVER
		gun->owner->dirty = true;
	#endif

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
	
	#ifdef CG_SERVER
		gun->owner->dirty = true;
	#endif
	}
	else
	{
	#ifdef CG_SERVER
		const f32 bullet_timer = gun->bullet_timer;
	#endif // CG_SERVER
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
	#ifdef CG_SERVER
		if (bullet_timer != gun->bullet_timer)
			gun->owner->dirty = true;
	#endif	// CG_SERVER
		return;
	}

#ifdef CG_SERVER
	const i32 start_ammo = gun->ammo;
#endif // CG_SERVER

	while (gun->bullet_timer >= gun->spec->bullet_spawn_interval && gun->ammo > 0)
	{
		cg_default_gun_shoot(cg, gun);
		gun->bullet_timer -= gun->spec->bullet_spawn_interval;
		gun->ammo--;
	}

#ifdef CG_SERVER
	if (start_ammo != gun->ammo)
		gun->owner->dirty = gun->owner->gun_dirty = true;
#endif // CG_SERVER

	if (gun->ammo <= 0 && cg->player_reload)
		cg->player_reload(gun->owner, cg->user_data);
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
coregame_player_change_gun_force(coregame_t* cg, cg_player_t *player, enum cg_gun_id id)
{
	if (player->gun)
		free(player->gun);

	return coregame_create_gun(cg, id, player) != NULL;
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
coregame_player_reload(coregame_t* cg, cg_player_t* player)
{
	if (player->gun == NULL || player->gun->ammo == player->gun->spec->max_ammo)
		return;

	player->gun->ammo = 0;

	if (cg->player_reload)
		cg->player_reload(player, cg->user_data);
}


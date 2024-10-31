#include "cg_map.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

cg_map_t*	
cg_map_load(const char* path);

static void
cg_map_set_cells_pos(cg_map_t* map)
{
	cg_cell_t* cell;

	for (u32 x = 0; x < map->header.w; x++)
	{
		for (u32 y = 0; y < map->header.h; y++)
		{
			cell = &map->cells[(y * map->header.w) + x];
			cell->pos.x = x;
			cell->pos.y = y;
			cell->type = CG_CELL_EMPTY;
		}
	}
}

cg_map_t*	
cg_map_new(u32 w, u32 h, u32 grid_size)
{
	cg_map_t* map;
	cg_cell_t* cell;
	u32 size = ((w * h) * sizeof(cg_cell_t)) + sizeof(cg_map_header_t);

	map = calloc(1, size);
	memcpy(map->header.magic, CG_MAP_MAGIC, CG_MAP_MAGIC_LEN);
	map->header.w = w;
	map->header.h = h;
	map->header.grid_size = grid_size;

	cg_map_set_cells_pos(map);

	// Default cells.
	cell = cg_map_at(map, 3, 3);
	cell->type = CG_CELL_BLOCK;
	cell = cg_map_at(map, 3, 4);
	cell->type = CG_CELL_BLOCK;
	cell = cg_map_at(map, 3, 5);
	cell->type = CG_CELL_BLOCK;

	cell = cg_map_at(map, 1, 1);
	cell->type = CG_CELL_SPAWN;

	return map;
}

bool		
cg_map_save(cg_map_t* map, const char* path);

cg_cell_t*	
cg_map_at(cg_map_t* map, u32 x, u32 y)
{
	if (x >= map->header.w || y >= map->header.h)
		return NULL;

	return &map->cells[(y * map->header.w) + x];
}

cg_cell_t*
cg_map_at_wpos(cg_map_t* map, const vec2f_t* pos)
{
	u32 x = (u32)pos->x / map->header.grid_size;
	u32 y = (u32)pos->y / map->header.grid_size;

	return cg_map_at(map, x, y);
}

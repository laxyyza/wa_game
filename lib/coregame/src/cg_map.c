#include "cg_map.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "array.h"

u64
file_size(FILE* f)
{
    u64 ret;
    fseek(f, 0, SEEK_END);
    ret = ftell(f);
    fseek(f, 0, SEEK_SET);
    return ret;
}

cg_map_t*	
cg_map_load(const char* path)
{
	cg_map_t* ret = NULL;
	cg_map_t* disk_map;
	u64 disk_size;
	u64 disk_cells_count = 0;
	cg_cell_t* disk_cell;
	cg_cell_t* cell;

	FILE* f = fopen(path, "rb");
	if (f == NULL)
	{
		perror("fopen");
		return NULL;
	}

	disk_size = file_size(f);
	disk_map = calloc(1, disk_size);
	fread(disk_map, 1, disk_size, f);

	if (memcmp(disk_map->header.magic, CG_MAP_MAGIC, CG_MAP_MAGIC_LEN))
	{
		fprintf(stderr, "Loading cgmap (%s) FAILED: Magic mismatch.\n", path);
		goto err;
	}
	disk_cells_count = (disk_size - sizeof(cg_map_header_t)) / sizeof(cg_cell_t);

	ret = cg_map_new(disk_map->header.w, disk_map->header.h, disk_map->header.grid_size);

	for (u32 i = 0; i < disk_cells_count; i++)
	{
		disk_cell = disk_map->cells + i;
		cell = cg_map_at(ret, disk_cell->pos.x, disk_cell->pos.y);
		memcpy(cell, disk_cell, sizeof(cg_cell_t));
	}
	printf("cgmap (%s) loaded. disk_size: %zu, disk_cells_count: %zu\n",
				path, disk_size, disk_cells_count);
err:
	fclose(f);
	free(disk_map);
	return ret;
}

static void
cg_map_set_cells_pos(cg_map_t* map)
{
	cg_cell_t* cell;

	for (u16 x = 0; x < map->header.w; x++)
	{
		for (u16 y = 0; y < map->header.h; y++)
		{
			cell = &map->cells[(y * map->header.w) + x];
			cell->pos.x = x;
			cell->pos.y = y;
			cell->type = CG_CELL_EMPTY;
		}
	}
}

cg_map_t*	
cg_map_new(u16 w, u16 h, u16 grid_size)
{
	cg_map_t* map;
	u32 size = ((w * h) * sizeof(cg_cell_t)) + sizeof(cg_map_header_t);

	map = calloc(1, size);
	memcpy(map->header.magic, CG_MAP_MAGIC, CG_MAP_MAGIC_LEN);
	map->header.w = w;
	map->header.h = h;
	map->header.grid_size = grid_size;

	cg_map_set_cells_pos(map);

	return map;
}

static u64 
cg_map_disk_size(const cg_map_t* map, array_t* idx_array)
{
	u64 size = sizeof(cg_map_header_t);
	u32 cells_count = map->header.w * map->header.h;
	const cg_cell_t* cell;

	for (u32 i = 0; i < cells_count; i++)
	{
		cell = map->cells + i;
		if (cell->type)
		{
			size += sizeof(cg_cell_t);
			array_add_i32(idx_array, i);
		}
	}
	return size;
}

bool		
cg_map_save(const cg_map_t* map, const char* path)
{
	FILE* f = fopen(path, "wb+");
	if (f == NULL)
	{
		perror("fopen");
		return false;
	}

	array_t idx_array;
	array_init(&idx_array, sizeof(u32), map->header.w * map->header.h);
	u32* idx_buf = (u32*)idx_array.buf;
	u64 disk_size = cg_map_disk_size(map, &idx_array);

	if (idx_array.count == 0)
		return false;

	cg_map_t* map_disk = calloc(1, disk_size);
	memcpy(&map_disk->header, &map->header, sizeof(cg_map_header_t));

	for (u32 i = 0; i < idx_array.count; i++)
	{
		cg_cell_t* dest_cell = map_disk->cells + i;
		const cg_cell_t* src_cell = map->cells + idx_buf[i];

		memcpy(dest_cell, src_cell, sizeof(cg_cell_t));
	}
	array_del(&idx_array);

	fwrite(map_disk, disk_size, 1, f);

	fclose(f);

	free(map_disk);

	return true;
}

cg_cell_t*	
cg_map_at(cg_map_t* map, u16 x, u16 y)
{
	if (x >= map->header.w || y >= map->header.h)
		return NULL;

	return &map->cells[(y * map->header.w) + x];
}

cg_cell_t*
cg_map_at_wpos(cg_map_t* map, const vec2f_t* pos)
{
	if (pos->x < 0 || pos->y < 0)
		return NULL;

	i32 x = (i32)pos->x / map->header.grid_size;
	i32 y = (i32)pos->y / map->header.grid_size;

	return cg_map_at(map, x, y);
}

static i32 
clampi(i32 val, i32 min, i32 max)
{
	if (val < min) return min;
	if (val > max) return max;
	return val;
}

cg_cell_t*
cg_map_at_wpos_clamp(cg_map_t* map, const vec2f_t* pos)
{
	i32 x = (i32)pos->x / map->header.grid_size;
	i32 y = (i32)pos->y / map->header.grid_size;

	x = clampi(x, 0, map->header.w - 1);
	y = clampi(y, 0, map->header.h - 1);

	return cg_map_at(map, x, y);
}

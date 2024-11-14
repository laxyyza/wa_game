#include "cg_map.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "array.h"
#include "cutils.h"

u64
file_size(FILE* f)
{
    u64 ret;
    fseek(f, 0, SEEK_END);
    ret = ftell(f);
    fseek(f, 0, SEEK_SET);
    return ret;
}

cg_runtime_map_t*	
cg_map_load(const char* path, cg_disk_map_t** disk_map_p, u32* disk_size_p)
{
	cg_runtime_map_t* ret = NULL;
	cg_disk_map_t* disk_map;
	u64 disk_size;

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
	fclose(f);

	ret = cg_map_load_disk(disk_map, disk_size);

	if (disk_map_p)
	{
		*disk_map_p = disk_map;
		*disk_size_p = disk_size;
	}
	else
		free(disk_map);
	return ret;
err:
	fclose(f);
	free(disk_map);
	return ret;
}

cg_runtime_map_t* 
cg_map_load_disk(const cg_disk_map_t* disk_map, u32 disk_size)
{
	cg_runtime_map_t* ret;
	u64 disk_cells_count = 0;
	const cg_disk_cell_t* disk_cell;
	cg_runtime_cell_t* runtime_cell;

	disk_cells_count = (disk_size - sizeof(cg_map_header_t)) / sizeof(cg_disk_cell_t);

	ret = cg_map_new(disk_map->header.w, disk_map->header.h, disk_map->header.grid_size);

	for (u32 i = 0; i < disk_cells_count; i++)
	{
		disk_cell = disk_map->cells + i;
		runtime_cell = cg_runtime_map_at(ret, disk_cell->pos.x, disk_cell->pos.y);
		runtime_cell->type = disk_cell->type;
		runtime_cell->pos = disk_cell->pos;
	}

	for (u32 x = 0; x < ret->w; x++)
	{
		for (u32 y = 0; y < ret->h; y++)
		{
			runtime_cell = cg_runtime_map_at(ret, x, y);
			if (runtime_cell->type == CG_CELL_BLOCK)
				runtime_cell->data = calloc(1, sizeof(cg_block_cell_data_t));
			else
			{
				runtime_cell->data = calloc(1, sizeof(cg_empty_cell_data_t));
				cg_empty_cell_data_t* data = runtime_cell->data;
				array_init(&data->contents, sizeof(cg_cell_data_t), 2);
			}
		}
	}

	array_init(&ret->runtime.edge_pool, sizeof(cg_line_t), 16);

	cg_map_compute_edge_pool(ret);
	return ret;
}

static void
cg_map_set_cells_pos(cg_runtime_map_t* map)
{
	cg_runtime_cell_t* cell;

	for (u16 x = 0; x < map->w; x++)
	{
		for (u16 y = 0; y < map->h; y++)
		{
			cell = &map->cells[(y * map->w) + x];
			cell->pos.x = x;
			cell->pos.y = y;
		}
	}
}

u32
cg_runtime_map_calc_size(u16 w, u16 h)
{
	return ((w * h) * sizeof(cg_runtime_cell_t)) + sizeof(cg_runtime_map_t);
}

u32
cg_disk_map_calc_size(u16 w, u16 h)
{
	return ((w * h) * sizeof(cg_disk_cell_t)) + sizeof(cg_map_header_t);
}

u32
cg_runtime_map_size(const cg_runtime_map_t* map)
{
	return cg_runtime_map_calc_size(map->w, map->h);
}

cg_runtime_map_t*	
cg_map_new(u16 w, u16 h, u16 grid_size)
{
	cg_runtime_map_t* map;
	u32 size = cg_runtime_map_calc_size(w, h);

	map = calloc(1, size);
	map->w = w;
	map->h = h;
	map->grid_size = grid_size;
	array_init(&map->runtime.edge_pool, sizeof(cg_line_t), 16);

	cg_map_set_cells_pos(map);

	return map;
}

static u16 
mini16(u16 a, u16 b)
{
	if (a > b)
		return b;
	else
		return a;
}

void
cg_map_resize(cg_runtime_map_t** mapp, u16 new_w, u16 new_h)
{
	cg_runtime_map_t* map = *mapp;
	cg_runtime_map_t* new_map;
	if(map->editor_mode == false)
		return;
	if (new_w == map->w && new_h == map->h)
		return;

	u16 old_w = map->w;
	u16 old_h = map->h;

	new_map = cg_map_new(new_w, new_h, map->grid_size);

	cg_runtime_cell_t* new_cell;
	cg_runtime_cell_t* old_cell;

	u16 safe_w = mini16(new_w, old_w);
	u16 safe_h = mini16(new_h, old_h);

	for (u16 x = 0; x < safe_w; x++)
	{
		for (u16 y = 0; y < safe_h; y++)
		{
			old_cell = &map->cells[(y * old_w) + x];
			new_cell = &new_map->cells[(y * new_w) + x];
			new_cell->type = old_cell->type;
		}
	}
	free(map);
	*mapp = new_map;
}

static u64 
cg_map_runtime_disk_size(const cg_runtime_map_t* map, array_t* idx_array)
{
	u64 size = sizeof(cg_map_header_t);
	u32 cells_count = map->w * map->h;
	const cg_runtime_cell_t* cell;

	for (u32 i = 0; i < cells_count; i++)
	{
		cell = map->cells + i;
		if (cell->type)
		{
			size += sizeof(cg_disk_cell_t);
			array_add_i32(idx_array, i);
		}
	}
	return size;
}

bool		
cg_map_save(const cg_runtime_map_t* map, const char* path)
{
	FILE* f = fopen(path, "wb+");
	if (f == NULL)
	{
		perror("fopen");
		return false;
	}

	array_t idx_array;
	array_init(&idx_array, sizeof(u32), map->w * map->h);
	u32* idx_buf = (u32*)idx_array.buf;
	u64 disk_size = cg_map_runtime_disk_size(map, &idx_array);

	if (idx_array.count == 0)
		return false;

	cg_disk_map_t* map_disk = calloc(1, disk_size);
	memcpy(map_disk->header.magic, CG_MAP_MAGIC, CG_MAP_MAGIC_LEN);
	map_disk->header.w = map->w;
	map_disk->header.h = map->h;
	map_disk->header.grid_size = map->grid_size;

	for (u32 i = 0; i < idx_array.count; i++)
	{
		cg_disk_cell_t* dest_cell = map_disk->cells + i;
		const cg_runtime_cell_t* src_cell = map->cells + idx_buf[i];

		dest_cell->type = src_cell->type;
		dest_cell->pos = src_cell->pos;
	}
	array_del(&idx_array);

	fwrite(map_disk, disk_size, 1, f);

	fclose(f);

	free(map_disk);

	return true;
}

cg_disk_cell_t*	
cg_disk_map_at(cg_disk_map_t* map, u16 x, u16 y)
{
	if (x >= map->header.w || y >= map->header.h)
		return NULL;

	return &map->cells[(y * map->header.w) + x];
}

cg_runtime_cell_t*	
cg_runtime_map_at(cg_runtime_map_t* map, u16 x, u16 y)
{
	if (x >= map->w || y >= map->h)
		return NULL;

	return &map->cells[(y * map->w) + x];
}

cg_runtime_cell_t*
cg_map_at_wpos(cg_runtime_map_t* map, const vec2f_t* pos)
{
	if (pos->x < 0 || pos->y < 0)
		return NULL;

	i32 x = (i32)pos->x / (i32)map->grid_size;
	i32 y = (i32)pos->y / (i32)map->grid_size;

	return cg_runtime_map_at(map, x, y);
}

cg_runtime_cell_t*
cg_map_at_wpos_clamp(cg_runtime_map_t* map, const vec2f_t* pos)
{
	i32 x = (i32)pos->x / (i32)map->grid_size;
	i32 y = (i32)pos->y / (i32)map->grid_size;

	x = clampi(x, 0, map->w - 1);
	y = clampi(y, 0, map->h - 1);

	return cg_runtime_map_at(map, x, y);
}

void
cg_runtime_map_free(cg_runtime_map_t* map)
{
	if (map == NULL)
		return;

	for (u32 x = 0; x < map->w; x++)
	{
		for (u32 y = 0; y < map->h; y++)
		{
			cg_runtime_cell_t* cell = cg_runtime_map_at(map, x, y);
			if (cell->data == NULL)
				continue;
			if (cell->type != CG_CELL_BLOCK)
			{
				cg_empty_cell_data_t* data = cell->data;
				array_del(&data->contents);
			}
			free(cell->data);
		}
	}
	array_del(&map->runtime.edge_pool);
	free(map);
}

static cg_runtime_cell_t* 
cg_runtime_map_at_edge(cg_runtime_map_t* map, u16 x, u16 y)
{
	cg_runtime_cell_t* ret;

	ret = cg_runtime_map_at(map, x, y);
	if (ret && ret->type != CG_CELL_BLOCK)
		ret = NULL;
	return ret;
}

static void
cg_map_get_cell_edges(cg_runtime_map_t* map, cg_runtime_cell_t* cell)
{
	array_t* edge_pool = &map->runtime.edge_pool;
	cg_block_cell_data_t* cell_data = cell->data;
	cg_runtime_cell_t* left = NULL;
	cg_runtime_cell_t* top = NULL;
	cg_runtime_cell_t* right = NULL;
	cg_runtime_cell_t* bot = NULL;
	cg_line_t*	left_edge = NULL;
	cg_line_t*	top_edge = NULL;
	cg_line_t*	right_edge = NULL;
	cg_line_t*	bot_edge = NULL;
	cg_block_cell_data_t* left_data = NULL;
	cg_block_cell_data_t* top_data = NULL;

	if (cell->type != CG_CELL_BLOCK)
		return;

	left = cg_runtime_map_at_edge(map,	cell->pos.x - 1,	cell->pos.y);
	if (left)
		left_data = left->data;

	top = cg_runtime_map_at_edge(map,	cell->pos.x,		cell->pos.y - 1);
	if (top)
		top_data = top->data;

	right = cg_runtime_map_at_edge(map,	cell->pos.x + 1,	cell->pos.y);
	bot = cg_runtime_map_at_edge(map,	cell->pos.x,		cell->pos.y + 1);

	if (left == NULL)
	{
		if (top == NULL || top_data->left == NULL)
		{
			left_edge = array_add_into(edge_pool);

			left_edge->a.x = cell->pos.x * map->grid_size;
			left_edge->a.y = cell->pos.y * map->grid_size;

			left_edge->b.x = left_edge->a.x;
			left_edge->b.y = left_edge->a.y + map->grid_size;
		}
		else
		{
			left_edge = top_data->left;
			left_edge->b.y += map->grid_size;
		}
	}
	if (top == NULL)
	{
		if (left == NULL || left_data->top == NULL)
		{
			top_edge = array_add_into(edge_pool);

			top_edge->a.x = cell->pos.x * map->grid_size;
			top_edge->a.y = cell->pos.y * map->grid_size;

			top_edge->b.x = top_edge->a.x + map->grid_size;
			top_edge->b.y = top_edge->a.y;
		}
		else
		{
			top_edge = left_data->top;
			top_edge->b.x += map->grid_size;
		}
	}
	if (right == NULL)
	{
		if (top == NULL || top_data->right == NULL)
		{
			right_edge = array_add_into(edge_pool);

			right_edge->a.x = (cell->pos.x * map->grid_size) + map->grid_size;
			right_edge->a.y = cell->pos.y * map->grid_size;

			right_edge->b.x = right_edge->a.x;
			right_edge->b.y = right_edge->a.y + map->grid_size;
		}
		else
		{
			right_edge = top_data->right;
			right_edge->b.y += map->grid_size;
		}
	}
	if (bot == NULL)
	{
		if (left == NULL || left_data->bot == NULL)
		{
			bot_edge = array_add_into(edge_pool);

			bot_edge->a.x = cell->pos.x * map->grid_size;
			bot_edge->a.y = (cell->pos.y * map->grid_size) + map->grid_size;

			bot_edge->b.x = bot_edge->a.x + map->grid_size;
			bot_edge->b.y = bot_edge->a.y;
		}
		else
		{
			bot_edge = left_data->bot;
			bot_edge->b.x += map->grid_size;
		}
	}
	
	cell_data->left = left_edge;
	cell_data->right = right_edge;
	cell_data->top = top_edge;
	cell_data->bot = bot_edge;
}

void 
cg_map_compute_edge_pool(cg_runtime_map_t* map)
{
	array_clear(&map->runtime.edge_pool, true);
	for (u32 x = 0; x < map->w; x++)
	{
		for (u32 y = 0; y < map->h; y++)
		{
			cg_map_get_cell_edges(map, cg_runtime_map_at(map, x, y));
		}
	}
}

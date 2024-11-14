#ifndef _COREGAME_MAP_H_
#define _COREGAME_MAP_H_

#include "rect.h"
#include <stdio.h>
#include "cutils.h"

#define CG_MAP_MAGIC ".cgmap"
#define CG_MAP_MAGIC_LEN sizeof(CG_MAP_MAGIC)

#define CG_CELL_EMPTY 0
#define CG_CELL_BLOCK 1
#define CG_CELL_SPAWN 2
#define CG_CELL_DEADZ 3

#define CG_PACKED __attribute__((packed))

#define MAP_PATH "res/maps"

enum cg_cell_data_type 
{
	CG_CELL_DATA_PLAYER,
	CG_CELL_DATA_BULLET,
};

typedef struct 
{
	enum cg_cell_data_type	type;
	void*					ptr;
} cg_cell_data_t;

typedef struct 
{
	vec2f_t a;
	vec2f_t b;
} cg_line_t;

typedef struct 
{
	cg_line_t* top;
	cg_line_t* bot;
	cg_line_t* left;
	cg_line_t* right;
} cg_block_cell_data_t;

typedef struct 
{
	array_t contents;
} cg_empty_cell_data_t;

typedef struct 
{
	vec2u16_t	pos;
	u8			type;
} CG_PACKED cg_disk_cell_t;

typedef struct 
{
	vec2u16_t	pos;
	u8			type;
	void*		data;
} cg_runtime_cell_t;

typedef struct 
{
	char magic[CG_MAP_MAGIC_LEN];
	u16 w;
	u16 h;
	u16 grid_size;
} CG_PACKED cg_map_header_t;

typedef struct 
{
	cg_map_header_t header;
	cg_disk_cell_t cells[];
} CG_PACKED cg_disk_map_t;

typedef struct 
{
	array_t edge_pool;
} cg_runtime_map_data_t;

typedef struct 
{
	u32 w;
	u32 h;
	u32 grid_size;
	bool editor_mode;

	struct {
		array_t edge_pool;
	} runtime;

	cg_runtime_cell_t		cells[];
} cg_runtime_map_t;

cg_runtime_map_t*	cg_map_load(const char* path, cg_disk_map_t** disk_map, u32* disk_size);
cg_runtime_map_t*	cg_map_load_disk(const cg_disk_map_t* disk_map, u32 size);
cg_runtime_map_t*	cg_map_new(u16 w, u16 h, u16 grid_size);
void				cg_map_resize(cg_runtime_map_t** mapp, u16 new_w, u16 new_h);
bool				cg_map_save(const cg_runtime_map_t* map, const char* path);

cg_disk_cell_t*		cg_disk_map_at(cg_disk_map_t* map, u16 x, u16 y);
cg_runtime_cell_t*	cg_runtime_map_at(cg_runtime_map_t* map, u16 x, u16 y);

cg_runtime_cell_t*	cg_map_at_wpos(cg_runtime_map_t* map, const vec2f_t* pos);
cg_runtime_cell_t*	cg_map_at_wpos_clamp(cg_runtime_map_t* map, const vec2f_t* pos);
u32					cg_disk_map_calc_size(u16 w, u16 h);
u32					cg_runtime_map_calc_size(u16 w, u16 h);
u32					cg_map_size(const cg_runtime_map_t* map);
void				cg_runtime_map_free(cg_runtime_map_t* map);
void				cg_map_compute_edge_pool(cg_runtime_map_t* map);

u64			file_size(FILE* f);
u16			mini16(u16 a, u16 b);

#endif // _COREGAME_MAP_H_

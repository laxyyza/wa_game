#ifndef _COREGAME_MAP_H_
#define _COREGAME_MAP_H_

#include "rect.h"

#define CG_MAP_MAGIC ".cgmap"
#define CG_MAP_MAGIC_LEN sizeof(CG_MAP_MAGIC)

#define CG_CELL_EMPTY 0
#define CG_CELL_BLOCK 1
#define CG_CELL_SPAWN 2
#define CG_CELL_DEADZ 3

typedef struct 
{
	vec2i_t pos;
	u64		data;
	u8		type;
} cg_cell_t;

typedef struct 
{
	char magic[CG_MAP_MAGIC_LEN];
	u32 w;
	u32 h;
	u32 grid_size;
} cg_map_header_t;

typedef struct 
{
	cg_map_header_t header;
	cg_cell_t cells[];
} cg_map_t;

cg_map_t*	cg_map_load(const char* path);
cg_map_t*	cg_map_new(u32 w, u32 h, u32 grid_size);
bool		cg_map_save(cg_map_t* map, const char* path);
cg_cell_t*	cg_map_at(cg_map_t* map, u32 x, u32 y);

#endif // _COREGAME_MAP_H_

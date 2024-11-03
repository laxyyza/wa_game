#ifndef _COREGAME_MAP_H_
#define _COREGAME_MAP_H_

#include "rect.h"
#include <stdio.h>

#define CG_MAP_MAGIC ".cgmap"
#define CG_MAP_MAGIC_LEN sizeof(CG_MAP_MAGIC)

#define CG_CELL_EMPTY 0
#define CG_CELL_BLOCK 1
#define CG_CELL_SPAWN 2
#define CG_CELL_DEADZ 3

#define CG_PACKED __attribute__((packed))

typedef struct 
{
	vec2u16_t	pos;
	u8			type;
} CG_PACKED cg_cell_t;

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
	cg_cell_t cells[];
} CG_PACKED cg_map_t;

cg_map_t*	cg_map_load(const char* path);
cg_map_t*	cg_map_new(u16 w, u16 h, u16 grid_size);
void		cg_map_resize(cg_map_t** mapp, u16 new_w, u16 new_h);
bool		cg_map_save(const cg_map_t* map, const char* path);
cg_cell_t*	cg_map_at(cg_map_t* map, u16 x, u16 y);
cg_cell_t*	cg_map_at_wpos(cg_map_t* map, const vec2f_t* pos);
cg_cell_t*	cg_map_at_wpos_clamp(cg_map_t* map, const vec2f_t* pos);

u64			file_size(FILE* f);

#endif // _COREGAME_MAP_H_

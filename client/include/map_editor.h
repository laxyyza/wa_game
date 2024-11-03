#ifndef _WAAPP_MAP_EDITOR_H_
#define _WAAPP_MAP_EDITOR_H_

#include "coregame.h"
#include "wa.h"

typedef struct waapp waapp_t;

#define MAP_NAME_MAX 32
#define MAP_PATH_MAX 512

typedef struct 
{
	u32 id;
	cg_map_header_t header;
	char path[MAP_PATH_MAX];
	char name[MAP_NAME_MAX];
	i32 selected;
	cg_map_t* map;
} editor_map_header_t;

typedef struct 
{
	cg_map_t* map;
	const u8* mouse_map;
	f32 og_zoom_min;
	ght_t   maps;
	editor_map_header_t* map_selected;
	const char* cell_types[2];
	i32 selected_cell_type;
} waapp_map_editor_t;

void map_editor_init(waapp_t* app, waapp_map_editor_t* editor);
void map_editor_enter(waapp_t* app, waapp_map_editor_t* editor);
void map_editor_update(waapp_t* app, waapp_map_editor_t* editor);
i32  map_editor_event(waapp_t* app, const wa_event_t* ev);
void map_editor_exit(waapp_t* app, waapp_map_editor_t* editor);

#endif // _WAAPP_MAP_EDITOR_H_

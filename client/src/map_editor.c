#include "map_editor.h"
#include "app.h"
#include "opengl.h"
#include <nuklear.h>
#include "file.h"

static u32 map_header_iq_seq = 1;

static void
map_editor_read_header(waapp_map_editor_t* editor, const char* path)
{
	FILE* f;

	if (ght_get(&editor->maps, ght_hashstr(path)))
		return;

	f = fopen(path, "r");
	if (f == NULL)
		return;

	editor_map_header_t* map_header = calloc(1, sizeof(editor_map_header_t));

	if (fread(&map_header->header, 1, sizeof(cg_map_header_t), f) != sizeof(cg_map_header_t))
		goto err;

	if (memcmp(map_header->header.magic, CG_MAP_MAGIC, CG_MAP_MAGIC_LEN))
		goto err;

	strncpy(map_header->path, path, MAP_PATH_MAX);
	char* filename = basename(path);
	filename = strtok(filename, ".");

	strncpy(map_header->name, filename, MAP_NAME_MAX);
	map_header->id = map_header_iq_seq;
	map_header_iq_seq++;

	fclose(f);
	ght_insert(&editor->maps, map_header->id, map_header);
	return;
err:
	free(map_header);
	fclose(f);
}

static void
editor_get_map_headers(waapp_map_editor_t* editor)
{
	array_t* map_files = dir_files(MAP_PATH);
	const char** file_names = (const char**)map_files->buf;

	for (u32 i = 0; i < map_files->count; i++)
	{
		const char* filepath = file_names[i];
		map_editor_read_header(editor, filepath);
		free((void*)filepath);
	}

	array_del(map_files);
	free(map_files);
}

void 
map_editor_init(waapp_t* app, waapp_map_editor_t* editor)
{
	wa_state_t* state = wa_window_get_state(app->window);
	editor->mouse_map = state->mouse_map;

	ght_init(&editor->maps, 5, free);
}

void 
map_editor_enter(waapp_t* app, waapp_map_editor_t* editor)
{
	editor->og_zoom_min = app->min_zoom;
	app->min_zoom = 0.1;

	app->keybind.cam_move = WA_MOUSE_MIDDLE;

	editor_get_map_headers(editor);
}

static void
map_editor_add_block(waapp_t* app, waapp_map_editor_t* editor)
{
	vec2f_t mpos = screen_to_world(app, &app->mouse);
	cg_cell_t* cell = cg_map_at_wpos(editor->map, &mpos);
	if (cell)
	{
		cell->type = CG_CELL_BLOCK;
	}
}

static void
map_editor_del_block(waapp_t* app, waapp_map_editor_t* editor)
{
	vec2f_t mpos = screen_to_world(app, &app->mouse);
	cg_cell_t* cell = cg_map_at_wpos(editor->map, &mpos);
	if (cell)
	{
		cell->type = CG_CELL_EMPTY;
	}
}

static void
map_editor_handle_building(waapp_t* app, waapp_map_editor_t* editor)
{
	if (app->on_ui)
		return;

	if (editor->mouse_map[WA_MOUSE_LEFT])
		map_editor_add_block(app, editor);
	else if (editor->mouse_map[WA_MOUSE_RIGHT])
		map_editor_del_block(app, editor);
}

static bool
map_editor_save_current(waapp_map_editor_t* editor)
{
	bool ret;

	char path[MAP_PATH_MAX];
	snprintf(path, MAP_PATH_MAX - 1, MAP_PATH"/%s.cgmap", editor->map_selected->name);
	ret = cg_map_save(editor->map, path);
	return ret;
}

static void
map_editor_ui(waapp_t* app, waapp_map_editor_t* editor)
{
	struct nk_context* ctx = app->nk_ctx;
	nk_flags flags = NK_WINDOW_TITLE | NK_WINDOW_BORDER | NK_WINDOW_MINIMIZABLE;
	static char status_msg[128];
	struct nk_rect rect = nk_rect(0, 0, 300, app->ren.viewport.y);
	bool free_map = false;
	bool new_map = false;

	if (nk_begin(ctx, "Map Editor", rect, flags))
	{
		nk_layout_row_dynamic(ctx, 30, 1);

		if (nk_menu_begin_label(ctx, "Misc.", NK_TEXT_LEFT, nk_vec2(rect.w, 400)))
		{
			nk_layout_row_dynamic(ctx, 30, 1);
			if (nk_button_label(ctx, "Back to Main Menu"))
			{
				waapp_state_switch(app, &app->sm.states.main_menu);
			}

			if (app->cam.x || app->cam.y)
			{
				if (nk_button_label(ctx, "Reset Camera Position"))
				{
					app->cam.x = app->cam.y = 0;
					ren_set_view(&app->ren, &app->cam);
				}
			}

			if (nk_button_label(ctx, "Create/Load New Map"))
				new_map = true;
			if (nk_button_label(ctx, "Discard Current Map"))
				free_map = true;
			nk_menu_end(ctx);
		}


		i32 width = editor->map->header.w;
		i32 height = editor->map->header.h;
		i32 grid_size = editor->map->header.grid_size;

		nk_property_int(ctx, "Grid Width", 1, &width, UINT16_MAX, 1, 10);
		nk_property_int(ctx, "Grid Height", 1, &height, UINT16_MAX, 1, 10);
		nk_property_int(ctx, "Cell Size", 10, &grid_size, UINT16_MAX, 1, 10);

		if (width != editor->map->header.w || height != editor->map->header.h)
			cg_map_resize(&editor->map, width, height);
		editor->map->header.grid_size = (u16)grid_size;

		nk_label(ctx, "Map name:", NK_TEXT_LEFT);
		nk_edit_string_zero_terminated(ctx, 
			NK_EDIT_BOX | NK_EDIT_SIG_ENTER, editor->map_selected->name, MAP_NAME_MAX, nk_filter_ascii);

		if (nk_button_label(ctx, "Save"))
		{
			if (strnlen(editor->map_selected->name, MAP_NAME_MAX))
			{
				if (map_editor_save_current(editor))
					strncpy(status_msg, "Saved!", 64);
				else
					strncpy(status_msg, "Failed!", 64);
			}
			else
			{
				strncpy(status_msg, "Please enter name.", 64);
			}
		}

		if (*status_msg)
			nk_label(ctx, status_msg, NK_TEXT_CENTERED);

	}
	nk_end(ctx);
	if (new_map)
	{
		editor->map_selected->map = editor->map;
		editor->map = NULL;
		editor->map_selected = NULL;
	}
	else if (free_map)
	{
		if (editor->map_selected->path[0] == 0x00)
			ght_del(&editor->maps, editor->map_selected->id);
		free(editor->map);
		editor->map = NULL;
		editor->map_selected = NULL;
	}
}

static void
editor_reset_map_disk_selection(waapp_map_editor_t* editor)
{
	ght_t* maps = &editor->maps;

	GHT_FOREACH(editor_map_header_t* map_header, maps, {
		map_header->selected = false;
	});
}

void 
map_editor_update(waapp_t* app, waapp_map_editor_t* editor)
{
	struct nk_context* ctx = app->nk_ctx;
	waapp_move_cam(app);

	if (editor->map)
	{
		map_editor_handle_building(app, editor);
		map_editor_ui(app, editor);
		if (editor->map)
			waapp_render_map(app, editor->map, true);
	}
	else
	{
		const char* name = "Create/Load cgmap";
		if (nk_begin(ctx, name, nk_rect(300, 300, 700, 600), 
				  NK_WINDOW_TITLE | NK_WINDOW_BORDER | 
				  NK_WINDOW_MOVABLE | NK_WINDOW_CLOSABLE |
				  NK_WINDOW_SCALABLE))
		{
			nk_layout_row_template_begin(ctx, 30);
			nk_layout_row_template_push_static(ctx, 150);
			if (nk_button_label(ctx, "Create New Map"))
			{
				editor->map = cg_map_new(10, 10, 100);
				editor->map_selected = calloc(1, sizeof(editor_map_header_t));
				editor->map_selected->id = map_header_iq_seq;
				map_header_iq_seq++;
				ght_insert(&editor->maps, editor->map_selected->id, editor->map_selected);
			}

			if (editor->map_selected)
			{
				nk_layout_row_template_push_static(ctx, 300);
				char button_label[MAP_PATH_MAX + 5];
				snprintf(button_label, MAP_PATH_MAX + 5, "Load %s", editor->map_selected->path);
				if (nk_button_label(ctx, button_label))
				{
					if (editor->map_selected->map)
						editor->map = editor->map_selected->map;
					else
						editor->map = cg_map_load(editor->map_selected->path, NULL, NULL);
				}
			}
			nk_layout_row_template_end(ctx);

			nk_layout_row_dynamic(ctx, 500, 1);

			if (nk_group_begin(ctx, "maps", NK_WINDOW_BORDER))
			{
				nk_layout_row_dynamic(ctx, 20, 3);

				ght_t* maps = &editor->maps;

				struct nk_vec2 spacing = ctx->style.window.spacing;
				ctx->style.window.spacing = nk_vec2(0, 2);

				ctx->style.selectable.normal = nk_style_item_color(nk_rgba(0, 0, 0, 0));
				ctx->style.selectable.hover = nk_style_item_color(nk_rgba(0, 0, 0, 0));
				ctx->style.selectable.pressed = nk_style_item_color(nk_rgba(40, 40, 40, 250));

				nk_label(ctx, "NAME", NK_TEXT_CENTERED);
				nk_label(ctx, "PATH", NK_TEXT_CENTERED);
				nk_label(ctx, "SIZE", NK_TEXT_CENTERED);

				GHT_FOREACH(editor_map_header_t* map_header, maps, 
				{
					char mapinfo[32];
					bool pressed = false;
					bool selected = map_header->selected;
					const char* header_name = (*map_header->name) ? map_header->name : "<?>";
					const char* header_path = (*map_header->path) ? map_header->path: "<not-saved>";

					if (nk_selectable_label(ctx, header_name, NK_TEXT_CENTERED, &map_header->selected))
						pressed = true;
					if (nk_selectable_label(ctx, header_path, NK_TEXT_CENTERED, &map_header->selected))
						pressed = true;
					
					snprintf(mapinfo, 32, "%ux%u", map_header->header.w, map_header->header.h);
					if (nk_selectable_label(ctx, mapinfo, NK_TEXT_CENTERED, &map_header->selected))
						pressed = true;

					if (pressed)
					{
						if (selected)
							pressed = false;
						editor_reset_map_disk_selection(editor);
						editor->map_selected = NULL;
						if ((map_header->selected = pressed))
							editor->map_selected = map_header;
					}
				});

				ctx->style.window.spacing = spacing;

				nk_group_end(ctx);
			}
		}
		nk_end(ctx);
		if (nk_window_is_hidden(ctx, name))
			waapp_state_switch(app, &app->sm.states.main_menu);
	}

	ren_draw_batch(&app->ren);

	ren_bind_bro(&app->ren, app->line_bro);
	ren_draw_batch(&app->ren);
	ren_bind_bro(&app->ren, app->ren.default_bro);
}

i32  
map_editor_event(waapp_t* app, const wa_event_t* ev)
{
	if (ev->type == WA_EVENT_MOUSE_WHEEL)
		game_handle_mouse_wheel(app, &ev->wheel);
	return 0;
}

void 
map_editor_exit(waapp_t* app, waapp_map_editor_t* editor)
{
	app->min_zoom = editor->og_zoom_min;
}

void 
map_editor_cleanup(UNUSED waapp_t* app, waapp_map_editor_t* editor)
{
	ght_destroy(&editor->maps);
	if (editor->map)
		free(editor->map);
}

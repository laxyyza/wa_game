#include "map_editor.h"
#include "app.h"
#include "opengl.h"
#include <nuklear.h>

void 
map_editor_init(waapp_t* app, waapp_map_editor_t* editor)
{
	wa_state_t* state = wa_window_get_state(app->window);

	if ((editor->map = cg_map_load("res/test.cgmap")) == NULL)
		editor->map = cg_map_new(100, 100, 100);

	editor->mouse_map = state->mouse_map;
	app->keybind.cam_move = WA_MOUSE_MIDDLE;
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

void 
map_editor_update(waapp_t* app, waapp_map_editor_t* editor)
{
	struct nk_context* ctx = app->nk_ctx;
	waapp_move_cam(app);

	map_editor_handle_building(app, editor);

	waapp_render_map(app, editor->map);

	nk_flags flags = NK_WINDOW_TITLE;

	if (nk_begin(ctx, "Map Editor", nk_rect(0, 0, 300, app->ren.viewport.y), flags))
	{
		nk_layout_row_dynamic(ctx, 30, 1);
		if (nk_button_label(ctx, "Save"))
		{
			if (cg_map_save(editor->map, "res/test.cgmap"))
				printf("Saved\n");
			else
				printf("Failed Save\n");
		}
	}
	nk_end(ctx);

	ren_draw_batch(&app->ren);
}

i32  
map_editor_event(waapp_t* app, const wa_event_t* ev)
{
	if (ev->type == WA_EVENT_MOUSE_WHEEL)
		game_handle_mouse_wheel(app, &ev->wheel);
	return 0;
}

void 
map_editor_exit(UNUSED waapp_t* app, UNUSED waapp_map_editor_t* editor)
{
}

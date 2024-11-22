#define _GNU_SOURCE
#include "main_menu.h"
#include "app.h"
#include "nuklear.h"
#include <string.h>
#include <stdlib.h>

#define MM_STATE_NAME_REQUIRED "Username required."
#define MM_STATE_IP_REQUIRED "IP Address required."
#define MM_SAVE_DATA_PATH "res/lastinfo"

static void
mm_load_last(waapp_main_menu_t* mm)
{
	FILE* f;

	f = fopen(MM_SAVE_DATA_PATH, "r");
	if (f == NULL)
		return;

	fread(&mm->sd, 1, sizeof(main_menu_save_data_t), f); 
	
	fclose(f);
}

static void
mm_save(waapp_main_menu_t* mm)
{
	FILE* f;

	f = fopen(MM_SAVE_DATA_PATH, "w+");
	if (f == NULL)
		return;

	fwrite(&mm->sd, 1, sizeof(main_menu_save_data_t), f);
	
	fclose(f);
}

void* 
main_menu_init(waapp_t* app)
{
	waapp_main_menu_t* mm = calloc(1, sizeof(waapp_main_menu_t));
	mm_load_last(mm);

	const i32 layout[] = {
		VERTLAYOUT_F32, 4, // position,
		VERTLAYOUT_END
	};
	const bro_param_t param = {
		.draw_mode = DRAW_TRIANGLES,
		.max_vb_count = 8,
		.vert_path = "client/src/shaders/mm_vert.glsl",
		.frag_path = "client/src/shaders/mm_frag.glsl",
		.shader = NULL,
		.vertlayout = layout,
		.vertex_size = sizeof(vec4f_t),
		.draw_rect = main_menu_draw_rect,
		.draw_line = NULL
	};

	mm->bg_bro = ren_new_bro(&app->ren, &param);
	
	rect_init(&mm->bg_rect, vec2f(-1.0, -1.0), vec2f(2.0, 2.0), 0xFF00FFFF, NULL);

	mm->bg_speed = -0.2;

	return mm;
}

void 
main_menu_enter(waapp_t* app, waapp_main_menu_t* mm)
{
	ren_bind_bro(&app->ren, mm->bg_bro);
	app->cam.x = 0;
	app->cam.y = 0;
	ren_set_view(&app->ren, &app->cam);
	app->ren.scale.x = 1;
	app->ren.scale.y = 1;
	ren_set_scale(&app->ren, &app->ren.scale);

	shader_bind(&mm->bg_bro->shader);
	shader_uniform_vec2f(&mm->bg_bro->shader, "res", &app->ren.viewport);
}

static void 
mm_bg_update(waapp_t* app, waapp_main_menu_t* mm)
{
	nano_start_time(&app->timer);

	if (mm->speed_changed)
		mm->speed_changed = false;
	else
	{
		f32 seconds = app->timer.start_time_s;

		seconds *= mm->bg_speed;

		shader_bind(&mm->bg_bro->shader);
		shader_uniform1f(&mm->bg_bro->shader, "time", seconds);
	}

	ren_bind_bro(&app->ren, mm->bg_bro);
	ren_draw_rect(&app->ren, &mm->bg_rect);
	ren_draw_batch(&app->ren);

	ren_bind_bro(&app->ren, app->ren.default_bro);
}

static void
mm_ui_update(waapp_t* app, waapp_main_menu_t* mm)
{
	struct nk_context* ctx = app->nk_ctx;
	const char* window_name = "Connect to Game Server";

	nk_flags flags = NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | 
					NK_WINDOW_TITLE | NK_WINDOW_SCALABLE | NK_WINDOW_CLOSABLE;

	if (nk_begin(ctx, window_name, nk_rect(50, 50, 320, 400), flags))
	{
		nk_layout_row_dynamic(ctx, 30, 1);
		nk_label(ctx, "Username:", NK_TEXT_LEFT);
		nk_edit_string_zero_terminated(ctx, 
				  NK_EDIT_FIELD, mm->sd.username, INET6_ADDRSTRLEN, nk_filter_ascii);

		nk_label(ctx, "Server IP Address: ip_address[:port]", NK_TEXT_LEFT);
		nk_edit_string_zero_terminated(ctx, 
				 NK_EDIT_BOX | NK_EDIT_SIG_ENTER | NK_EDIT_CLIPBOARD, mm->sd.ipaddr, INET6_ADDRSTRLEN, nk_filter_ascii);

		if (nk_button_label(ctx, "Connect"))
		{
			u32 name_len = strnlen(mm->sd.username, PLAYER_NAME_MAX);
			u32 ip_len = strnlen(mm->sd.ipaddr, INET6_ADDRSTRLEN);
			if (name_len == 0)
				strncpy(mm->state, MM_STATE_NAME_REQUIRED, MM_STATE_STRING_MAX - 1);
			else if (ip_len == 0)
				strncpy(mm->state, MM_STATE_IP_REQUIRED, MM_STATE_STRING_MAX - 1);
			else
			{
				const char* ret = client_net_async_connect(app, mm->sd.ipaddr);
				strncpy(mm->state, ret, MM_STATE_STRING_MAX - 1);
			}
		}

		nk_label(ctx, mm->state, NK_TEXT_CENTERED);

		if (nk_button_label(ctx, "Map Editor"))
		{
			waapp_state_switch(app, &app->sm.states.map_editor);
		}

		char speed_str[64];
		snprintf(speed_str, 64, "Background speed: %f", mm->bg_speed);
		nk_label(ctx, speed_str, NK_TEXT_LEFT);
		if (nk_slider_float(ctx, -10.0, &mm->bg_speed, 10.0, 0.001))
			mm->speed_changed = true;
	}
	nk_end(ctx);
	if (nk_window_is_hidden(ctx, window_name))
		wa_window_stop(app->window);
}

void 
main_menu_update(waapp_t* app, waapp_main_menu_t* mm)
{
	mm_bg_update(app, mm);
	mm_ui_update(app, mm);
}

i32 
main_menu_event(UNUSED waapp_t* app, waapp_main_menu_t* mm, const wa_event_t* ev)
{
	if (ev->type == WA_EVENT_RESIZE)
	{
		vec2f_t res = {ev->resize.w, ev->resize.h};
		shader_bind(&mm->bg_bro->shader);
		shader_uniform_vec2f(&mm->bg_bro->shader, "res", &res);
		mm->bg_rect.size.x = res.x;
		mm->bg_rect.size.y = res.y;
	}

	return 1;
}

void 
main_menu_exit(UNUSED waapp_t* app, waapp_main_menu_t* mm)
{
	mm_save(mm);
	mm->state[0] = 0x00;
	ren_bind_bro(&app->ren, app->ren.default_bro);
}

void 
main_menu_cleanup(waapp_t* app, waapp_main_menu_t* mm)
{
	ren_delete_bro(&app->ren, mm->bg_bro);
	free(mm);
}

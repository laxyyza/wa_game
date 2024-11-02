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

void 
main_menu_init(UNUSED waapp_t* app, waapp_main_menu_t* mm)
{
	mm_load_last(mm);
}

void 
main_menu_enter(UNUSED waapp_t* app, waapp_main_menu_t* mm)
{
	mm->state[0] = 0x00;
}

void 
main_menu_update(waapp_t* app, waapp_main_menu_t* mm)
{
	struct nk_context* ctx = app->nk_ctx;

	nk_flags flags = NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | 
					NK_WINDOW_TITLE | NK_WINDOW_SCALABLE | NK_WINDOW_CLOSABLE;

	if (nk_begin(ctx, "Connect to Game Server", nk_rect(50, 50, 320, 400), flags))
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
	}
	nk_end(ctx);
}

i32 
main_menu_event(UNUSED waapp_t* app, UNUSED const wa_event_t* ev)
{
	return 1;
}

void 
main_menu_exit(UNUSED waapp_t* app, waapp_main_menu_t* mm)
{
	mm_save(mm);
}

void 
main_menu_cleanup(UNUSED waapp_t* app, UNUSED waapp_main_menu_t* mm)
{

}

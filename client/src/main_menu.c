#include "main_menu.h"
#include "app.h"
#include "nuklear.h"

void 
main_menu_init(UNUSED waapp_t* app, void* data)
{
	waapp_main_menu_t* mm = data;
	strncpy(mm->ipaddr, "127.0.0.1", INET6_ADDRSTRLEN);
	strncpy(mm->username, "username", PLAYER_NAME_MAX);
}

void 
main_menu_enter(UNUSED waapp_t* app, UNUSED void* data)
{

}

void 
main_menu_update(waapp_t* app, void* data)
{
	struct nk_context* ctx = app->nk_ctx;
	waapp_main_menu_t* mm = data;

	nk_flags flags = NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | 
					NK_WINDOW_TITLE | NK_WINDOW_SCALABLE | NK_WINDOW_CLOSABLE;

	if (nk_begin(ctx, "Connect to Game Server", nk_rect(50, 50, 300, 400), flags))
	{
		nk_layout_row_dynamic(ctx, 30, 1);
		nk_label(ctx, "Username:", NK_TEXT_LEFT);
		nk_edit_string_zero_terminated(ctx, 
				  NK_EDIT_FIELD, mm->username, INET6_ADDRSTRLEN, nk_filter_ascii);

		nk_label(ctx, "Server IP Address: ip_address[:port]", NK_TEXT_LEFT);
		nk_edit_string_zero_terminated(ctx, 
				 NK_EDIT_BOX | NK_EDIT_SIG_ENTER | NK_EDIT_CLIPBOARD, mm->ipaddr, INET6_ADDRSTRLEN, nk_filter_ascii);

		if (nk_button_label(ctx, "Connect"))
		{
			const char* ret = client_net_async_connect(app, mm->ipaddr);
			strncpy(mm->state, ret, MM_STATE_STRING_MAX);
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
main_menu_exit(UNUSED waapp_t* app, UNUSED void* data)
{

}

void 
main_menu_cleanup(UNUSED waapp_t* app, UNUSED void* data)
{

}

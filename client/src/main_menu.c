#include "main_menu.h"
#include "app.h"
#include "nuklear.h"

void 
main_menu_init(UNUSED waapp_t* app, UNUSED void* data)
{
	printf("main menu init\n");
}

void 
main_menu_update(waapp_t* app, UNUSED void* data)
{
	struct nk_context* ctx = app->nk_ctx;

	nk_flags flags = NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | 
					NK_WINDOW_TITLE | NK_WINDOW_SCALABLE;

	if (nk_begin(ctx, "Connect to Server", nk_rect(50, 50, 500, 500), flags))
	{
		nk_layout_row_dynamic(ctx, 30, 1);
		nk_label(ctx, "Main Menu", NK_TEXT_LEFT);
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

#ifdef __linux__
#include "app_wayland.h"
#include "app.h"
#include "wa_wayland.h"

static void
wayland_read(UNUSED waapp_t* app, fdevent_t* fdev)
{
	wa_window_t* window = fdev->data;
    if (wl_display_prepare_read(window->wl_display) == 0)
		wa_wayland_read_events(window);
    else
        wl_display_dispatch_pending(window->wl_display);
}

void 
waapp_wayland_add_fdevent(waapp_t* app)
{
	client_net_add_fdevent(app, app->window->display_fd, wayland_read, NULL, NULL, app->window);
}

#endif // __linux__

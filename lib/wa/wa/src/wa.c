#include "wa.h"
#include "wa_log.h"
#include <string.h>

#ifndef WA_DEFAULT_APP_ID
    #ifdef _WA_WAYLAND
        #define WA_DEFAULT_APP_ID "wa"
    #else
        #define WA_DEFAULT_APP_ID NULL
    #endif
#endif

#ifndef WA_DEFAULT_TITLE
    #ifdef _WA_WAYLAND
        #define WA_DEFAULT_TITLE "WA - Window Abstraction (Wayland)"
    #elif _WA_WIN32
        #define WA_DEFAULT_TITLE "WA - Window Abstraction (Win32)"
    #else
        #error "What platform? Wayland? Win32?"
    #endif
#endif

static void 
wa_default_event(_WA_UNUSED wa_window_t* window, _WA_UNUSED const wa_event_t* event, _WA_UNUSED void* data)
{
}

static void 
wa_default_update(_WA_UNUSED wa_window_t* window, _WA_UNUSED void* data)
{
}

static void 
wa_default_draw(_WA_UNUSED wa_window_t* window, _WA_UNUSED void* data)
{
}

static void 
wa_default_close(_WA_UNUSED wa_window_t* window, _WA_UNUSED void* data)
{
}

static void
wa_default_focus(_WA_UNUSED wa_window_t* window, _WA_UNUSED void* data)
{
}

static void
wa_default_unfocus(_WA_UNUSED wa_window_t* window, _WA_UNUSED void* data)
{
}

void 
wa_state_set_default(wa_state_t* state)
{
    if (!state)
    {
        wa_logf(WA_WARN, "state is NULL!\n");
        return;
    }

    memset(state, 0, sizeof(wa_state_t));

    state->callbacks.event = wa_default_event;
    state->callbacks.update = wa_default_update;
    state->callbacks.close = wa_default_close;
    state->callbacks.draw = wa_default_draw;
    state->callbacks.focus = wa_default_focus;
    state->callbacks.unfocus = wa_default_unfocus;
    state->window.w = 100;
    state->window.h = 100;
    state->window.title = WA_DEFAULT_TITLE;
    state->window.vsync = true;

    state->window.wayland.app_id = WA_DEFAULT_APP_ID;
    state->window.wayland.opaque = true; // Set this to false to have transparent window (Wayland)

    state->window.egl.red_size = 8;
    state->window.egl.green_size = 8;
    state->window.egl.blue_size = 8;
    state->window.egl.alpha_size = 8;
    state->window.egl.depth_size = 24;
}

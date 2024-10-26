#include "wa_wayland.h"
#include "wa_cursor.h"

#include <string.h>
#include "wa_log.h"
#include <errno.h>

#define WA_CMP(x) !strcmp(interface, x)

static void
wa_set_surface_opaque(wa_window_t* window)
{
    if (window->state.window.wayland.opaque == false)
        return;

    wl_region_add(window->wl_region, 0, 0, 
                  window->state.window.w, 
                  window->state.window.h);
    wl_surface_set_opaque_region(window->wl_surface, window->wl_region);
}

static void 
wa_window_resize(wa_window_t* window)
{
    const i32 w = window->state.window.w;
    const i32 h = window->state.window.h;

    if (window->wl_egl_window == NULL)
        return;

    wl_egl_window_resize(window->wl_egl_window, w, h, 0, 0);
    wa_set_surface_opaque(window);
    wa_log(WA_VBOSE, "Window resized: %dx%d\n", w, h);
}

static void 
wa_xdg_shell_ping(_WA_UNUSED void* data, struct xdg_wm_base* xdg_shell, u32 serial)
{
    xdg_wm_base_pong(xdg_shell, serial);
    wa_log(WA_VBOSE, "XDG Shell pong!\n");
}

static void 
wa_reg_glob(void* data, struct wl_registry* reg, u32 name, const char* interface, u32 version)
{
    wa_window_t* window = data;

    if (WA_CMP(wl_compositor_interface.name))
    {
        window->wl_compositor = wl_registry_bind(reg, name, &wl_compositor_interface, version);
    }
    else if (WA_CMP(xdg_wm_base_interface.name))
    {
        window->xdg_shell = wl_registry_bind(reg, name, &xdg_wm_base_interface, version);
        window->xdg_shell_listener.ping = wa_xdg_shell_ping;
        xdg_wm_base_add_listener(window->xdg_shell, &window->xdg_shell_listener, window);
    }
    else if (WA_CMP(wl_seat_interface.name))
    {
        window->wl_seat = wl_registry_bind(reg, name, &wl_seat_interface, version);
        window->wl_seat_listener.capabilities = wa_seat_cap;
        window->wl_seat_listener.name = wa_seat_name;
        wl_seat_add_listener(window->wl_seat, &window->wl_seat_listener, window);
    }
    else if (WA_CMP(wl_output_interface.name))
    {
        if (window->wl_outputs == NULL)
        {
            window->wl_outputs = calloc(1, sizeof(void*));
            window->n_outputs = 1;
        }
        else 
        {
            window->n_outputs++;
            window->wl_outputs = realloc(window->wl_outputs, sizeof(void*) * window->n_outputs);
        }

        window->wl_outputs[window->n_outputs - 1] = wl_registry_bind(reg, name, &wl_output_interface, version);
    }
    else if (WA_CMP(zxdg_decoration_manager_v1_interface.name))
    {
        window->xdg_decoration_manager = wl_registry_bind(reg, name, &zxdg_decoration_manager_v1_interface, version);
    }
    else if (WA_CMP(wp_tearing_control_manager_v1_interface.name))
    {
        window->tearing_manager = wl_registry_bind(reg, name, &wp_tearing_control_manager_v1_interface, version);
    }
    else if (WA_CMP(wp_cursor_shape_manager_v1_interface.name))
    {
        window->cursor_shape_manager = wl_registry_bind(reg, name, &wp_cursor_shape_manager_v1_interface, version);
    }
    else
    {
        wa_log(WA_VBOSE, "Interface: %u '%s' v%u\n", name, interface, version);
        return;
    }

    wa_log(WA_VBOSE, "Using Interface: %u '%s' v%u\n", name, interface, version);
}

static void 
wa_reg_glob_rm(_WA_UNUSED void* data, _WA_UNUSED struct wl_registry* reg, u32 name)
{
    wa_log(WA_WARN, "Interface: %u removed.\n", name);
}

static void 
wa_draw(wa_window_t* window)
{
    window->state.callbacks.draw(window, window->state.user_data);

    eglSwapBuffers(window->egl_display, window->egl_surface);
}

static void 
wa_frame_done(void* data, struct wl_callback* callback, _WA_UNUSED u32 callback_data)
{
    wa_window_t* window = data;

    wl_callback_destroy(callback);

    // If VSync is disabled, do the drawing in the mainloop.
    if (window->state.window.vsync == false)
        return;

    callback = wl_surface_frame(window->wl_surface);
    wl_callback_add_listener(callback, &window->wl_frame_done_listener, data);

    wa_draw(window);
}

static void 
wa_xdg_surface_conf(_WA_UNUSED void* data, struct xdg_surface* xdg_surface, u32 serial)
{
    xdg_surface_ack_configure(xdg_surface, serial);
    wa_log(WA_VBOSE, "XDG Surface configire\n");
}

static void 
wa_init_xdg_decoration(wa_window_t* window)
{
    if (window->xdg_decoration_manager)
    {
        window->xdg_toplevel_decoration = zxdg_decoration_manager_v1_get_toplevel_decoration(
                window->xdg_decoration_manager, 
                window->xdg_toplevel
        );
        zxdg_toplevel_decoration_v1_set_mode(
                window->xdg_toplevel_decoration, 
                ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE
        );
    }
}

static void 
wa_toplevel_conf(void* data, _WA_UNUSED struct xdg_toplevel* toplevel, i32 w, i32 h, struct wl_array* states)
{
    wa_window_t* window = data;

    if (w == 0 || h == 0)
        return;

    if (w != window->state.window.w || h != window->state.window.h)
    {
        window->state.window.w = w;
        window->state.window.h = h;

        wa_window_resize(window);

        wa_event_t event = {
            .type = WA_EVENT_RESIZE,
            .resize.w = w,
            .resize.h = h
        };

        window->state.callbacks.event(window, &event, window->state.user_data);
    }

    u16 window_state = 0;

    u32* state;
    wl_array_for_each(state, states)
    {
        switch (*state)
        {
            case XDG_TOPLEVEL_STATE_FULLSCREEN:
                window_state |= WA_STATE_FULLSCREEN;
                break;
            case XDG_TOPLEVEL_STATE_MAXIMIZED:
                window_state |= WA_STATE_MAXIMIZED;
                break;
            case XDG_TOPLEVEL_STATE_ACTIVATED:
                window_state |= WA_STATE_ACTIVE;
                break;
        }
    }
    window->state.window.state = window_state;
    wa_log(WA_VBOSE, "XDG Toplevel configure\n");
}

static void 
wa_toplevel_close(void* data, _WA_UNUSED struct xdg_toplevel* toplevel)
{
    wa_window_t* window = data;

    window->state.callbacks.close(window, window->state.user_data);

    window->running = false;

    wa_log(WA_DEBUG, "XDG Toplevel close\n");
}

static void 
wa_toplevel_conf_bounds(_WA_UNUSED void* data, _WA_UNUSED struct xdg_toplevel* toplevel, _WA_UNUSED i32 w, _WA_UNUSED i32 h)
{
    // wa_log(WA_DEBUG, "toplevel bounds: %dx%d\n", w, h);
}

static void 
wa_toplevel_wm_caps(_WA_UNUSED void* data, _WA_UNUSED struct xdg_toplevel* toplevel, _WA_UNUSED struct wl_array* caps)
{
    u32* cap;
    wl_array_for_each(cap, caps)
    {
        switch (*cap)
        {
            case XDG_TOPLEVEL_WM_CAPABILITIES_MAXIMIZE:
                wa_log(WA_VBOSE, "CAP: MAXIMIZE!\n");
                break;
            case XDG_TOPLEVEL_WM_CAPABILITIES_MINIMIZE:
                wa_log(WA_VBOSE, "CAP: MINIMIZE!\n");
                break;
            case XDG_TOPLEVEL_WM_CAPABILITIES_FULLSCREEN:
                wa_log(WA_VBOSE, "CAP: FULLSCREEN!\n");
                break;
            case XDG_TOPLEVEL_WM_CAPABILITIES_WINDOW_MENU:
                wa_log(WA_VBOSE, "CAP: WINDOW MENU!\n");
                break;
            default:
                wa_log(WA_VBOSE, "CAP: Unknown %u\n", *cap);
                break;
        }
    }
}

static void 
wa_output_geo(_WA_UNUSED void* data, _WA_UNUSED struct wl_output* output, 
              i32 x, i32 y, i32 phy_w, i32 phy_h, i32 subpixel, 
              const char* make, const char* model, i32 transform)
{
    wa_log(WA_VBOSE, "Output\n\tx/y:\t%dx%d\n\tphy w/h:\t%dx%d\n\tsubpixel:\t%d\n\tmake:\t'%s'\n\tmodel:\t'%s'\n\ttransform:\t%d\n",
            x, y, phy_w, phy_h, subpixel, make, model, transform);
}

static void 
wa_output_mode(_WA_UNUSED void* data, _WA_UNUSED struct wl_output* output, u32 flags, i32 w, i32 h, i32 refresh_rate)
{
    wa_log(WA_VBOSE, "Mode: %dx%d @ %d (flags: %u)\n", w, h, refresh_rate, flags);
}

static void 
wa_output_done(_WA_UNUSED void* data, _WA_UNUSED struct wl_output* output)
{
    wa_log(WA_VBOSE, "%s()\n", __func__);
}

static void 
wa_output_scale(_WA_UNUSED void* data, _WA_UNUSED struct wl_output* output, i32 factor)
{
    wa_log(WA_VBOSE, "Scale:\t%d\n", factor);
}

static void 
wa_output_name(_WA_UNUSED void* data, _WA_UNUSED struct wl_output* output, const char* name)
{
    wa_log(WA_VBOSE, "Name:\t'%s'\n", name);
}

static void 
wa_output_desc(_WA_UNUSED void* data, _WA_UNUSED struct wl_output* output, const char* desc)
{
    wa_log(WA_VBOSE, "Desc:\t'%s'\n", desc);
}

static bool 
wa_window_init_wayland(wa_window_t* window)
{
    window->cursor_shape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_DEFAULT;

    if ((window->wl_display = wl_display_connect(NULL)) == NULL)
    {
        wa_logf(WA_FATAL, "Failed to connect to Wayland display!\n");
        return false;
    }

    if ((window->wl_registry = wl_display_get_registry(window->wl_display)) == NULL)
    {
        wa_logf(WA_FATAL, "Failed to get registry!\n");
        return false;
    }
    window->wl_registry_listener.global = wa_reg_glob;
    window->wl_registry_listener.global_remove = wa_reg_glob_rm;
    wl_registry_add_listener(window->wl_registry, &window->wl_registry_listener, window);
    if (wl_display_roundtrip(window->wl_display) == -1)
        wa_log(WA_ERROR, "wl_display_roundtrip() failed!\n");

    /* wl_surface */
    if ((window->wl_surface = wl_compositor_create_surface(window->wl_compositor)) == NULL)
    {
        wa_logf(WA_FATAL, "Failed to create wl_surface!\n");
        return false;
    }
    if ((window->frame_done_callback = wl_surface_frame(window->wl_surface)) == NULL)
    {
        wa_logf(WA_FATAL, "Failed to create wl_callback for wl_surface!\n");
        return false;
    }
    window->wl_frame_done_listener.done = wa_frame_done;
    wl_callback_add_listener(window->frame_done_callback, &window->wl_frame_done_listener, window);

    /* wl_region */
    if (window->state.window.wayland.opaque && (window->wl_region = wl_compositor_create_region(window->wl_compositor)) == NULL)
    {
        wa_logf(WA_FATAL, "Failed to create wl_region!!\n");
        return false;
    }
    wa_set_surface_opaque(window);

    /* XDG Surface */
    if ((window->xdg_surface = xdg_wm_base_get_xdg_surface(window->xdg_shell, window->wl_surface)) == NULL)
    {
        wa_logf(WA_FATAL, "Failed to create xdg_surface!\n");
        return false;
    }
    window->xdg_surface_listener.configure = wa_xdg_surface_conf;
    xdg_surface_add_listener(window->xdg_surface, &window->xdg_surface_listener, window);

    /* XDG Toplevel */
    if ((window->xdg_toplevel = xdg_surface_get_toplevel(window->xdg_surface)) == NULL)
    {
        wa_logf(WA_FATAL, "Failed to get XDG Toplevel!\n");
        return false;
    }
    window->xdg_toplevel_listener.configure = wa_toplevel_conf;
    window->xdg_toplevel_listener.close = wa_toplevel_close;
    window->xdg_toplevel_listener.configure_bounds = wa_toplevel_conf_bounds;
    window->xdg_toplevel_listener.wm_capabilities = wa_toplevel_wm_caps;
    xdg_toplevel_add_listener(window->xdg_toplevel, &window->xdg_toplevel_listener, window);

    const char* title = window->state.window.title;
    xdg_toplevel_set_title(window->xdg_toplevel, title);

    const char* app_id = window->state.window.wayland.app_id;
    xdg_toplevel_set_app_id(window->xdg_toplevel, app_id);

    wa_window_set_fullscreen(window, window->state.window.state & WA_STATE_FULLSCREEN);

    /* wl_output */
    window->wl_output_listener.geometry = wa_output_geo;
    window->wl_output_listener.mode = wa_output_mode;
    window->wl_output_listener.done = wa_output_done;
    window->wl_output_listener.scale = wa_output_scale;
    window->wl_output_listener.name = wa_output_name;
    window->wl_output_listener.description = wa_output_desc;
    for (u64 i = 0; i < window->n_outputs; i++)
        wl_output_add_listener(window->wl_outputs[i], &window->wl_output_listener, window);

    wa_init_xdg_decoration(window);

    if (window->tearing_manager)
        window->tearing = wp_tearing_control_manager_v1_get_tearing_control(window->tearing_manager, window->wl_surface);
    else
    {
        wa_log(WA_DEBUG, "Wayland Compositor doesn't support tearing control.\n");
        window->state.window.vsync = true;
    }

    wl_surface_commit(window->wl_surface);
    wl_display_roundtrip(window->wl_display);

    window->display_fd = wl_display_get_fd(window->wl_display);
    window->pollfd.fd = window->display_fd;
    window->pollfd.events = POLLIN;

    return true;
}

wa_window_t* 
wa_window_create(const char* title, i32 w, i32 h, bool fullscreen)
{
    wa_state_t state;
    wa_state_set_default(&state);
    state.window.title = title;
    state.window.state = (fullscreen) ? WA_STATE_FULLSCREEN : 0;
    state.window.w = w;
    state.window.h = h;

    return wa_window_create_from_state(&state);
}

wa_window_t* 
wa_window_create_from_state(wa_state_t* state)
{
    if (!state)
    {
        wa_logf(WA_FATAL, "%s() state is NULL!\n", __func__);
        return NULL;
    }
    
    wa_window_t* window = malloc(sizeof(wa_window_t));
    if (window == NULL)
    {
        wa_logf(WA_FATAL, "malloc() returned NULL!\n");
        return NULL;
    }
    memset(window, 0, sizeof(wa_window_t));

    memcpy(&window->state, state, sizeof(wa_state_t));

    if (!wa_window_init_wayland(window))
    {
        wa_window_delete(window);
        return NULL;
    }

    if (!wa_window_egl_init(window))
    {
        wa_window_delete(window);
        return NULL;
    }

    wa_window_vsync(window, window->state.window.vsync);

    window->xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

    wl_display_dispatch(window->wl_display);

    wa_draw(window);

    window->running = true;
    wa_log(WA_INFO, "Window \"%s\" %dx%d created\n", window->state.window.title, window->state.window.w, window->state.window.h);

    return window;
}

void 
wa_window_set_callbacks(wa_window_t* window, const wa_callbacks_t* callbacks)
{
    if (!window || !callbacks)
        return;

    memcpy(&window->state.callbacks, callbacks, sizeof(wa_callbacks_t));
}

wa_state_t* 
wa_window_get_state(wa_window_t* window)
{
    return &window->state;
}

bool
wa_window_running(const wa_window_t* window)
{
    return window->running;
}

static void 
wa_wayland_poll(wa_window_t* window, i32 timeout)
{
    i32 ret = poll(&window->pollfd, 1, timeout);
    if (ret > 0)
    {
        wl_display_read_events(window->wl_display);
        wl_display_dispatch_pending(window->wl_display);
    }
    else if (ret == 0)
    {
        wl_display_cancel_read(window->wl_display);
    }
    else
    {
        wa_log(WA_ERROR, "poll: %s\n", 
               strerror(errno));
        window->running = false;
    }
}

void
wa_window_poll_timeout(wa_window_t* window, i32 timeout)
{
    if (wl_display_prepare_read(window->wl_display) == 0)
        wa_wayland_poll(window, timeout);
    else
        wl_display_dispatch_pending(window->wl_display);
}

void
wa_window_poll(wa_window_t* window)
{
    wa_window_poll_timeout(window, window->poll_timeout);
}

int 
wa_window_mainloop(wa_window_t* window)
{
    if (!window)
    {
        wa_logf(WA_FATAL, "%s() window is NULL!\n", __func__);
        return -1;
    }

    if (!window->state.callbacks.close || !window->state.callbacks.event || !window->state.callbacks.update)
    {
        wa_logf(WA_FATAL, "%s() state callback(s) are NULL!", __func__);
        return -1;
    }

    while (window->running)
    {
        wa_window_poll(window);

        window->state.callbacks.update(window, window->state.user_data);
        if (window->state.window.vsync == false)
            wa_draw(window);
    }

    return 0;
}

void 
wa_window_set_fullscreen(wa_window_t* window, bool fullscreen)
{
    if (!window)
    {
        wa_logf(WA_WARN, "%s() window is NULL!\n", __func__);
        return;
    }

    if (fullscreen)
        xdg_toplevel_set_fullscreen(window->xdg_toplevel, NULL);
    else
        xdg_toplevel_unset_fullscreen(window->xdg_toplevel);
}

void 
wa_window_stop(wa_window_t* window)
{
    if (!window)
    {
        wa_logf(WA_WARN, "%s() window is NULL!\n", __func__);
        return;
    }
    window->running = false;
}

static void 
wa_window_xkb_cleanup(wa_window_t* window)
{
    if (window->xkb_context)
        xkb_context_unref(window->xkb_context);
    if (window->xkb_keymap)
        xkb_keymap_unref(window->xkb_keymap);
    if (window->xkb_state)
        xkb_state_unref(window->xkb_state);
}

static void 
wa_window_wayland_cleanup(wa_window_t* window)
{
    if (window->tearing)
        wp_tearing_control_v1_destroy(window->tearing);
    if (window->tearing_manager)
        wp_tearing_control_manager_v1_destroy(window->tearing_manager);
    if (window->xdg_toplevel_decoration)
        zxdg_toplevel_decoration_v1_destroy(window->xdg_toplevel_decoration);
    if (window->xdg_decoration_manager)
        zxdg_decoration_manager_v1_destroy(window->xdg_decoration_manager);
    if (window->xdg_toplevel)
        xdg_toplevel_destroy(window->xdg_toplevel);
    if (window->xdg_surface)
        xdg_surface_destroy(window->xdg_surface);
    if (window->xdg_shell)
        xdg_wm_base_destroy(window->xdg_shell);
    if (window->wl_registry)
        wl_registry_destroy(window->wl_registry);
    if (window->wl_compositor)
        wl_compositor_destroy(window->wl_compositor);
    if (window->wl_surface)
        wl_surface_destroy(window->wl_surface);
    if (window->wl_keyboard)
        wl_keyboard_destroy(window->wl_keyboard);
    if (window->wl_pointer)
        wl_pointer_destroy(window->wl_pointer);
    if (window->wl_seat)
        wl_seat_destroy(window->wl_seat);
    if (window->wl_region)
        wl_region_destroy(window->wl_region);
    for (u64 i = 0; i < window->n_outputs; i++)
        if (window->wl_outputs[i])
            wl_output_destroy(window->wl_outputs[i]);

    if (window->wl_display)
        wl_display_disconnect(window->wl_display);
}

void 
wa_window_delete(wa_window_t* window)
{
    if (!window)
        return;

    wa_window_egl_cleanup(window);
    wa_window_xkb_cleanup(window);
    wa_window_wayland_cleanup(window);
        
    free(window);

    wa_log(WA_DEBUG, "Cleanup done.\n");
}

static void 
wa_wayland_tear(wa_window_t* window, bool tear)
{
    enum wp_tearing_control_v1_presentation_hint hint;

    if (window->tearing == NULL)
        return; // This means compositor doesn't support tearing control.

    if (tear)
        hint = WP_TEARING_CONTROL_V1_PRESENTATION_HINT_ASYNC;
    else
    {
        hint = WP_TEARING_CONTROL_V1_PRESENTATION_HINT_VSYNC;

        if (window->state.window.vsync == false)
        {
            // Use the frame callback again.
            window->frame_done_callback = wl_surface_frame(window->wl_surface);
            wl_callback_add_listener(window->frame_done_callback, 
                                     &window->wl_frame_done_listener, 
                                     window);
            
            wa_draw(window);
        }
    }

    wp_tearing_control_v1_set_presentation_hint(window->tearing, hint);
}

void 
wa_window_vsync(wa_window_t* window, bool vsync)
{
    eglSwapInterval(window->egl_display, vsync);
    wa_wayland_tear(window, !vsync);
    window->state.window.vsync = vsync;
    window->poll_timeout = (vsync) ? -1 : 0;

    wa_log(WA_DEBUG, "Set VSync: %d\n", vsync);
}

#include "wa_event.h"
#include "wa_wayland.h"
#include "wa_wayland_xkbcommon.h"
#include "wa_cursor.h"
#include "wa_log.h"
#include "xdg-shell.h"
#include <linux/input-event-codes.h>

void 
wa_kb_map(void* data, _WA_UNUSED struct wl_keyboard* keyboard, 
          _WA_UNUSED u32 frmt, i32 fd, u32 size)
{
    wa_window_t* window = data;
    wa_xkb_map(window, size, fd);
}

void 
wa_kb_enter(_WA_UNUSED void* data, _WA_UNUSED struct wl_keyboard* keyboard, 
            u32 serial, _WA_UNUSED struct wl_surface* surface, 
            _WA_UNUSED struct wl_array* array)
{
    wa_window_t* window = data;
    wa_log(WA_VBOSE, "kb_enter: serial: %u\n", serial);
    window->state.callbacks.focus(window, window->state.user_data);
}

void 
wa_kb_leave(_WA_UNUSED void* data, _WA_UNUSED struct wl_keyboard* keyboard, 
            u32 serial, _WA_UNUSED struct wl_surface* surface)
{
    wa_window_t* window = data;
    wa_log(WA_VBOSE, "kb_leave: serial: %u\n", serial);
    window->state.callbacks.unfocus(window, window->state.user_data);
}

void 
wa_kb_key(void* data, _WA_UNUSED struct wl_keyboard* keyboard, 
          _WA_UNUSED u32 serial, _WA_UNUSED u32 t, 
          u32 key, u32 state)
{
    wa_window_t* window = data;
    wa_key_t wa_key = wa_xkb_key(window, key, state);

    wa_event_t key_event = {
        .type = WA_EVENT_KEYBOARD,
        .keyboard.pressed = state,
        .keyboard.key = wa_key
    };

    window->state.callbacks.event(window, &key_event, window->state.user_data);
}

void 
wa_kb_mod(void* data, 
          _WA_UNUSED struct wl_keyboard* keyboard, 
          _WA_UNUSED u32 serial,
          u32 mods_depressed, u32 mods_latched, 
          u32 mods_locked, u32 group)
{
    wa_window_t* window = data;

    xkb_state_update_mask(window->xkb_state, mods_depressed, mods_latched, 
                          mods_locked, 0, 0, group);
}

void 
wa_kb_rep(_WA_UNUSED void* data, _WA_UNUSED struct wl_keyboard* keyboard, 
          i32 rate, i32 del)
{
    wa_log(WA_VBOSE, "kb_rep: rate: %u, del: %d\n", rate, del);
}

void 
wa_point_enter(void* data, struct wl_pointer* pointer, 
               u32 serial, _WA_UNUSED struct wl_surface* surface, 
               wl_fixed_t surface_x, wl_fixed_t surface_y)
{
    wa_window_t* window = data;
    wa_log(WA_VBOSE, "WA: point_enter(surxy: %dx%d)\n", surface_x, surface_y);

    window->pointer_serial = serial;
    if (window->cursor_shape_device)
        wp_cursor_shape_device_v1_destroy(window->cursor_shape_device);
    window->cursor_shape_device = wp_cursor_shape_manager_v1_get_pointer(window->cursor_shape_manager, pointer);
    wa_set_cursor_shape(window, window->wa_cursor_shape);
}

void 
wa_point_leave(_WA_UNUSED void* data, _WA_UNUSED struct wl_pointer* pointer, 
               _WA_UNUSED u32 serial, _WA_UNUSED struct wl_surface* surface)
{
    wa_log(WA_VBOSE, "WA: point_leave()\n");
}

void 
wa_point_move(void* data, _WA_UNUSED struct wl_pointer* pointer, _WA_UNUSED u32 time, 
              wl_fixed_t surface_x, wl_fixed_t surface_y)
{
    wa_window_t* window = data;
    const i32 x = wl_fixed_to_int(surface_x);
    const i32 y = wl_fixed_to_int(surface_y);

    wa_event_t event = {
        .type = WA_EVENT_POINTER,
        .pointer.x = x,
        .pointer.y = y,
    };

    window->state.callbacks.event(window, &event, window->state.user_data);
}

static wa_mouse_butt_t
wa_mouse_button(u32 button)
{
    switch (button)
    {
        case BTN_LEFT:
            return WA_MOUSE_LEFT;
        case BTN_RIGHT:
            return WA_MOUSE_RIGHT;
        case BTN_MIDDLE:
            return WA_MOUSE_MIDDLE;
        default:
            return WA_MOUSE_UNKOWN;
    }
}

void 
wa_point_button(void* data, _WA_UNUSED struct wl_pointer* pointer, 
                u32 serial, u32 time, u32 button, u32 state)
{
    wa_window_t* window = data;

    wa_log(WA_VBOSE, "WA: point_button(serial: %u, time: %u) button: %u, state: %u\n", 
           serial, time, button, state);

    bool pressed = state == 1;
    wa_mouse_butt_t mbut = wa_mouse_button(button);

    window->state.mouse_map[mbut] = pressed;

    wa_event_t ev = {
        .type = WA_EVENT_MOUSE_BUTTON,
        .mouse.button = mbut,
        .mouse.pressed = pressed
    };
    window->state.callbacks.event(window, &ev, window->state.user_data);
}

void 
wa_point_axis(_WA_UNUSED void* data, _WA_UNUSED struct wl_pointer* pointer, 
              _WA_UNUSED u32 time, _WA_UNUSED u32 axis_type, 
              _WA_UNUSED wl_fixed_t value)
{
    /* no-op */
}

void 
wa_point_frame(_WA_UNUSED void* data, _WA_UNUSED struct wl_pointer* pointer)
{
    /* no-op */
}

void 
wa_point_axis_src(_WA_UNUSED void* data, _WA_UNUSED struct wl_pointer* pointer, 
                  _WA_UNUSED u32 axis_src)
{
    /* no-op */
}

void 
wa_point_axis_stop(_WA_UNUSED void* data, _WA_UNUSED struct wl_pointer* pointer, 
                   _WA_UNUSED u32 time, _WA_UNUSED u32 axis)
{
    /* no-op */
}

void 
wa_point_axis_discrete(_WA_UNUSED void* data, _WA_UNUSED struct wl_pointer* pointer, 
                       _WA_UNUSED u32 axis, _WA_UNUSED i32 discrete)
{
    /* no-op */
}

void 
wa_point_axis120(void* data, _WA_UNUSED struct wl_pointer* pointer, 
                 u32 axis_type, i32 value)
{
    wa_window_t* window = data;

    wa_event_t ev = {
        .type = WA_EVENT_MOUSE_WHEEL,
        .wheel.value = value,
        .wheel.axis = axis_type
    };
    window->state.callbacks.event(window, &ev, window->state.user_data);
}

void 
wa_point_axis_dir(_WA_UNUSED void* data, _WA_UNUSED struct wl_pointer* pointer, 
                  _WA_UNUSED u32 axis_type, _WA_UNUSED u32 dir)
{
    /* no-op */
}

void 
wa_seat_cap(void* data, struct wl_seat* seat, u32 cap)
{
    wa_window_t* window = data;
    if (cap & WL_SEAT_CAPABILITY_KEYBOARD)
    {
        window->wl_keyboard = wl_seat_get_keyboard(seat);

        window->wl_keyboard_listener.keymap = wa_kb_map;
        window->wl_keyboard_listener.enter = wa_kb_enter;
        window->wl_keyboard_listener.leave = wa_kb_leave;
        window->wl_keyboard_listener.key = wa_kb_key;
        window->wl_keyboard_listener.modifiers = wa_kb_mod;
        window->wl_keyboard_listener.repeat_info = wa_kb_rep;

        wl_keyboard_add_listener(window->wl_keyboard, &window->wl_keyboard_listener, data);
    }
    if (cap & WL_SEAT_CAPABILITY_POINTER)
    {
        window->wl_pointer = wl_seat_get_pointer(seat);

        window->wl_pointer_listener.enter = wa_point_enter;
        window->wl_pointer_listener.leave = wa_point_leave;
        window->wl_pointer_listener.motion = wa_point_move;
        window->wl_pointer_listener.button = wa_point_button;
        window->wl_pointer_listener.axis = wa_point_axis;
        window->wl_pointer_listener.frame = wa_point_frame;
        window->wl_pointer_listener.axis_source = wa_point_axis_src;
        window->wl_pointer_listener.axis_stop = wa_point_axis_stop;
        window->wl_pointer_listener.axis_discrete = wa_point_axis_discrete;
        window->wl_pointer_listener.axis_value120 = wa_point_axis120;
        window->wl_pointer_listener.axis_relative_direction = wa_point_axis_dir;

        wl_pointer_add_listener(window->wl_pointer, &window->wl_pointer_listener, window);
    }
}

void 
wa_seat_name(_WA_UNUSED void* data, _WA_UNUSED struct wl_seat* seat, const char* name)
{
    wa_log(WA_VBOSE, "wl_seat name: '%s'\n", name);
}

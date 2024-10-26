#include "wa_cursor.h"
#include "wa_wayland.h"

static enum wp_cursor_shape_device_v1_shape
wa_wayland_cursor(enum wa_cursor_shape shape)
{
    switch (shape)
    {
        case WA_CURSOR_DEFAULT:
            return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_DEFAULT;
        case WA_CURSOR_CONTEXT_MENU:
            return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_CONTEXT_MENU;
        case WA_CURSOR_HELP:
            return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_HELP;
        case WA_CURSOR_POINTER:
            return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_POINTER;
        case WA_CURSOR_PROGRESS:
            return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_PROGRESS;
        case WA_CURSOR_WAIT:
            return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_WAIT;
        case WA_CURSOR_CELL:
            return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_CELL;
        case WA_CURSOR_CROSSHAIR:
            return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_CROSSHAIR;
        case WA_CURSOR_TEXT:
            return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_TEXT;
        case WA_CURSOR_VERTICAL_TEXT:
            return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_VERTICAL_TEXT;
        case WA_CURSOR_ALIAS:
            return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_ALIAS;
        case WA_CURSOR_COPY:
            return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_COPY;
        case WA_CURSOR_MOVE:
            return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_MOVE;
        case WA_CURSOR_NO_DROP:
            return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_NO_DROP;
        case WA_CURSOR_NOT_ALLOWED:
            return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_NOT_ALLOWED;
        case WA_CURSOR_GRAB:
            return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_GRAB;
        case WA_CURSOR_GRABBING:
            return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_GRABBING;
        case WA_CURSOR_E_RESIZE:
            return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_E_RESIZE;
        case WA_CURSOR_N_RESIZE:
            return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_N_RESIZE;
        case WA_CURSOR_NE_RESIZE:
            return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_NE_RESIZE;
        case WA_CURSOR_NW_RESIZE:
            return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_NW_RESIZE;
        case WA_CURSOR_S_RESIZE:
            return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_S_RESIZE;
        case WA_CURSOR_SE_RESIZE:
            return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_SE_RESIZE;
        case WA_CURSOR_SW_RESIZE:
            return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_SW_RESIZE;
        case WA_CURSOR_W_RESIZE:
            return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_W_RESIZE;
        case WA_CURSOR_EW_RESIZE:
            return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_EW_RESIZE;
        case WA_CURSOR_NS_RESIZE:
            return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_NS_RESIZE;
        case WA_CURSOR_NESW_RESIZE:
            return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_NESW_RESIZE;
        case WA_CURSOR_NWSE_RESIZE:
            return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_NWSE_RESIZE;
        case WA_CURSOR_COL_RESIZE:
            return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_COL_RESIZE;
        case WA_CURSOR_ROW_RESIZE:
            return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_ROW_RESIZE;
        case WA_CURSOR_ALL_SCROLL:
            return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_ALL_SCROLL;
        case WA_CURSOR_ZOOM_IN:
            return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_ZOOM_IN;
        case WA_CURSOR_ZOOM_OUT:
            return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_ZOOM_OUT;
        default:
            return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_DEFAULT;
    }
}

static void
wa_wayland_set_cursor(wa_window_t* window, enum wa_cursor_shape shape)
{
    enum wp_cursor_shape_device_v1_shape wp_shape = wa_wayland_cursor(shape);

    wp_cursor_shape_device_v1_set_shape(window->cursor_shape_device, 
                                        window->pointer_serial, 
                                        wp_shape);
    window->cursor_shape = wp_shape;
}

void 
wa_set_cursor_shape(wa_window_t* window, enum wa_cursor_shape shape)
{
    if (window->cursor_shape_manager == NULL)
        return;

    wa_wayland_set_cursor(window, shape);
    window->wa_cursor_shape = shape;
}

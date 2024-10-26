#include "wa_cursor.h"
#include "wa_win32.h"

static LPCSTR
wa_win32_cursor(enum wa_cursor_shape shape)
{
    switch (shape)
    {
        case WA_CURSOR_DEFAULT:
            return IDC_ARROW;
        // case WA_CURSOR_CONTEXT_MENU:
        //     return 
        case WA_CURSOR_HELP:
            return IDC_HELP;
        // case WA_CURSOR_POINTER:
        //     return 
        // case WA_CURSOR_PROGRESS:
        //     return 
        case WA_CURSOR_WAIT:
            return IDC_WAIT;
        // case WA_CURSOR_CELL:
        //     return 
        case WA_CURSOR_CROSSHAIR:
            return IDC_CROSS;
        // case WA_CURSOR_TEXT:
        //     return 
        case WA_CURSOR_VERTICAL_TEXT:
            return IDC_IBEAM;
        // case WA_CURSOR_ALIAS:
        //     return 
        // case WA_CURSOR_COPY:
        //     return 
        // case WA_CURSOR_MOVE:
        //     return 
        // case WA_CURSOR_NO_DROP:
        //     return 
        case WA_CURSOR_NOT_ALLOWED:
            return IDC_NO;
        case WA_CURSOR_GRAB:
            return IDC_HAND;
        // case WA_CURSOR_GRABBING:
        //     return
        // case WA_CURSOR_E_RESIZE:
        //     return 
        // case WA_CURSOR_N_RESIZE:
        //     return 
        // case WA_CURSOR_NE_RESIZE:
        //     return 
        // case WA_CURSOR_NW_RESIZE:
        //     return 
        // case WA_CURSOR_S_RESIZE:
        //     return 
        // case WA_CURSOR_SE_RESIZE:
        //     return 
        // case WA_CURSOR_SW_RESIZE:
        //     return 
        // case WA_CURSOR_W_RESIZE:
        //     return 
        // case WA_CURSOR_EW_RESIZE:
        //     return 
        // case WA_CURSOR_NS_RESIZE:
        //     return 
        case WA_CURSOR_NESW_RESIZE:
            return IDC_SIZENESW;
        case WA_CURSOR_NWSE_RESIZE:
            return IDC_SIZENWSE;
        // case WA_CURSOR_COL_RESIZE:
        //     return 
        // case WA_CURSOR_ROW_RESIZE:
        //     return 
        // case WA_CURSOR_ALL_SCROLL:
        //     return 
        // case WA_CURSOR_ZOOM_IN:
        //     return 
        // case WA_CURSOR_ZOOM_OUT:
        //     return 
        default:
            return IDC_ARROW;
    }
}

void
wa_set_cursor_shape(wa_window_t* window, enum wa_cursor_shape shape)
{
    LPCSTR win32_cursor = wa_win32_cursor(shape);

    window->wc.hCursor = LoadCursor(NULL, win32_cursor);
    SetCursor(window->wc.hCursor);
}

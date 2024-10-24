#ifndef _WA_WIN32_H_
#define _WA_WIN32_H_

#include "wa.h"
#include "wa_log.h"
#include <GL/gl.h>
#include <GL/wglext.h>
#include <windows.h>

#define KEY_COUNT 256

typedef struct wa_window
{
    HWND hwnd;
    WNDCLASS wc;
    HINSTANCE instance;
    RECT rect;
    HDC hdc;
    HGLRC hglrc;

    const char* class_name;

    wa_state_t state;

    bool running;
} wa_window_t;

#endif // _WA_WIN32_H_

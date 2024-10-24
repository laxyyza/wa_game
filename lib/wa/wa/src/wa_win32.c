#include "wa_win32.h"
#include "wa.h"
#include "wa_event.h"
#include <stdio.h>
#include <windows.h>
#include "wa_win32_keymap.h"

wa_window_t*    
wa_window_create(const char* title, i32 w, i32 h, bool fullscreen)
{
    wa_state_t state;
    wa_state_set_default(&state);
    state.window.title = title;
    state.window.w = w;
    state.window.h = h;
    state.window.state = (fullscreen) ? WA_STATE_FULLSCREEN : 0;
    return wa_window_create_from_state(&state);
}

static void
wa_draw(wa_window_t* window)
{
    window->state.callbacks.draw(window, window->state.user_data);

    SwapBuffers(window->hdc);
}

static void
wa_resize(wa_window_t* window, i32 w, i32 h)
{
    if (w == 0 || h == 0)
        return;

    window->state.window.w = w;
    window->state.window.h = h;

    wa_event_t ev = {
        .type = WA_EVENT_RESIZE,
        .resize.w = w,
        .resize.h = h
    };
    window->state.callbacks.event(window, &ev, window->state.user_data);
}

static void
wa_win32_keyevent(wa_window_t* window, UINT type, WPARAM wparam)
{
    bool pressed = (type == WM_KEYDOWN);
    wa_key_t key = wa_win32_key_to_wa_key(wparam);

    window->state.key_map[key] = pressed;

    wa_event_t ev = {
        .type = WA_EVENT_KEYBOARD,
        .keyboard.key = key,
        .keyboard.pressed = pressed
    };
    window->state.callbacks.event(window, &ev, window->state.user_data);
}

static void
wa_win32_button_event(wa_window_t* window, UINT msg)
{
    wa_mouse_butt_t button;
    bool pressed = false;

    switch (msg)
    {
        case WM_LBUTTONDOWN:
            pressed = true;
            button = WA_MOUSE_LEFT;
            break;
        case WM_LBUTTONUP:
            button = WA_MOUSE_LEFT;
            break;
        case WM_RBUTTONDOWN:
            pressed = true;
            button = WA_MOUSE_RIGHT;
            break;
        case WM_RBUTTONUP:
            button = WA_MOUSE_RIGHT;
            break;
        case WM_MBUTTONDOWN:
            pressed = true;
            button = WA_MOUSE_MIDDLE;
            break;
        case WM_MBUTTONUP:
            button = WA_MOUSE_MIDDLE;
            break;
        default:
            button = WA_MOUSE_UNKOWN;
    }

    window->state.mouse_map[button] = pressed;

    wa_event_t ev = {
        .type = WA_EVENT_MOUSE_BUTTON,
        .mouse.button = button,
        .mouse.pressed = pressed
    };
    window->state.callbacks.event(window, &ev, window->state.user_data);
}

LRESULT CALLBACK 
window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    wa_window_t* window = (wa_window_t*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    switch (msg)
    {
        case WM_CLOSE:
            DestroyWindow(hwnd);
            wa_log(WA_DEBUG, "WM_CLOSE\n");
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            wa_log(WA_DEBUG, "WM_DESTROY\n");
            return 0;
        case WM_SIZE:
        {
            i32 w = LOWORD(lparam);
            i32 h = HIWORD(lparam);
            wa_log(WA_VBOSE, "WM_SIZE: %d/%d\n", w, h);
            wa_resize(window, w, h);
            return 0;
        }
        case WM_SETFOCUS:
        {
            window->state.callbacks.focus(window, window->state.user_data);
            return 0;
        }
        case WM_KILLFOCUS:
        {
            window->state.callbacks.unfocus(window, window->state.user_data);
            return 0;
        }
        case WM_KEYDOWN:
        case WM_KEYUP:
            wa_win32_keyevent(window, msg, wparam);
            return 0;
        case WM_MOUSEMOVE:
        {
            wa_event_t ev = {
                .type = WA_EVENT_POINTER,
                .pointer.x = LOWORD(lparam),
                .pointer.y = HIWORD(lparam),
            };
            window->state.callbacks.event(window,
                                          &ev,
                                          window->state.user_data);
            return 0;
        }
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
            wa_win32_button_event(window, msg);
            return 0;
        case WM_PAINT:
            wa_draw(window);
            return 0;
        case WM_MOUSEWHEEL:
        {
            wa_event_t ev = {
                .type = WA_EVENT_MOUSE_WHEEL,
                .wheel.value = GET_WHEEL_DELTA_WPARAM(wparam),
            };
            window->state.callbacks.event(window, &ev, window->state.user_data);
            return 0;
        }
        case WM_SETCURSOR:
        {
            if (LOWORD(lparam) == HTCLIENT) 
            {
                SetCursor(window->wc.hCursor); 
                return TRUE;
            }
            return 0;
        }
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

// Define the function pointer type
typedef HGLRC (APIENTRY *wglCreateContextAttribsARBProc)(HDC, HGLRC, const i32 *);

// Declare a global variable for the function pointer
wglCreateContextAttribsARBProc wglCreateContextAttribsARB = NULL;

wa_window_t*    
wa_window_create_from_state(wa_state_t* state)
{
    wa_window_t* window;

    window = calloc(1, sizeof(wa_window_t));
    memcpy(&window->state, state, sizeof(wa_state_t));
    window->instance = GetModuleHandle(NULL);
    window->class_name = "class name?";

    WNDCLASS* wc = &window->wc;
    wc->lpfnWndProc = window_proc;
    wc->hInstance = window->instance;
    wc->lpszClassName = window->class_name;
    wc->hIcon = LoadIcon(NULL, IDI_WINLOGO);
    wc->hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(wc);

    DWORD style = WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU;
    RECT* rect = &window->rect;
    rect->left = 250;
    rect->top = 250;
    rect->right = rect->left + window->state.window.w;
    rect->bottom = rect->top + window->state.window.h;

    AdjustWindowRect(rect, style, false);

    window->hwnd = CreateWindowEx(
        0, 
        window->class_name, 
        state->window.title,
        style,
        rect->left, 
        rect->top, 
        rect->right - rect->left, 
        rect->bottom - rect->top,
        NULL, 
        NULL, 
        window->instance, 
        NULL
    );
    if (window->hwnd == NULL)
    {
        wa_logf(WA_FATAL, "CreateWindowEx() return NULL!\n");
        free(window);
        return NULL;
    }

    SetWindowLongPtr(window->hwnd, GWLP_USERDATA, (LONG_PTR)window);

    ShowWindow(window->hwnd, SW_SHOWDEFAULT);
    UpdateWindow(window->hwnd);

    wa_window_set_fullscreen(window, window->state.window.state & WA_STATE_FULLSCREEN);

    HDC hdc = GetDC(window->hwnd);
    window->hdc = hdc;

    PIXELFORMATDESCRIPTOR pfd = {0};
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.iLayerType = PFD_MAIN_PLANE;

    i32 pixelformat = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, pixelformat, &pfd);

    // Create a temporary OpenGL context
    HGLRC tempContext = wglCreateContext(hdc);
    wglMakeCurrent(hdc, tempContext);

// Retrieve the function pointer
    wglCreateContextAttribsARB = (wglCreateContextAttribsARBProc)wglGetProcAddress("wglCreateContextAttribsARB");

    if (wglCreateContextAttribsARB) 
    {
        // Define attributes for the core profile context
        i32 attribs[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
            WGL_CONTEXT_MINOR_VERSION_ARB, 6,
            WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
            0
        };

        HGLRC coreContext = wglCreateContextAttribsARB(hdc, 0, attribs);
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(tempContext);
        wglMakeCurrent(hdc, coreContext);
        window->hglrc = coreContext;
    }
    else
        wa_log(WA_ERROR, "NUULL?\n");

    wa_window_vsync(window, window->state.window.vsync);

    window->running = true;

    return window;
}

wa_state_t*     
wa_window_get_state(wa_window_t* window)
{
    return &window->state;
}

void
wa_window_poll_timeout(wa_window_t *window, _WA_UNUSED i32 timeout)
{
    MSG msg;
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            wa_log(WA_DEBUG, "WM_QUIT\n");
            window->running = false;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void 
wa_window_poll(wa_window_t* window)
{
    wa_window_poll_timeout(window, 0);
}

int     
wa_window_mainloop(wa_window_t* window)
{
    while (window->running)
    {
        wa_window_poll(window);

        window->state.callbacks.update(window, window->state.user_data);
        // wa_draw(window);
    }
    window->state.callbacks.close(window, window->state.user_data);
    return 0;
}

void    
wa_window_set_fullscreen(wa_window_t* window, bool fullscreen)
{
    DWORD style;

    if (fullscreen) 
    {
        // Save the current window size and position
        GetWindowRect(window->hwnd, &window->rect);

        // Switch to fullscreen mode
        style = WS_POPUP;
        SetWindowLong(window->hwnd, GWL_STYLE, style);
        SetWindowLong(window->hwnd, GWL_EXSTYLE, WS_EX_APPWINDOW | WS_EX_TOPMOST);

        // Get the screen size
        window->rect.left = 0;
        window->rect.top = 0;
        window->rect.right = GetSystemMetrics(SM_CXSCREEN);
        window->rect.bottom = GetSystemMetrics(SM_CYSCREEN);

        // Set the window to cover the entire screen
        SetWindowPos(window->hwnd, HWND_TOPMOST,
                     window->rect.left, window->rect.top,
                     window->rect.right - window->rect.left,
                     window->rect.bottom - window->rect.top,
                     SWP_NOACTIVATE | SWP_NOREDRAW | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        window->state.window.state |= WA_STATE_FULLSCREEN;
    } 
    else 
    {
        // Switch to windowed mode
        style = WS_OVERLAPPEDWINDOW;
        SetWindowLong(window->hwnd, GWL_STYLE, style);

        // Restore the window's previous size and position
        SetWindowPos(window->hwnd, HWND_TOPMOST,
                     window->rect.left, window->rect.top,
                     window->rect.right - window->rect.left,
                     window->rect.bottom - window->rect.top,
                     SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        window->state.window.state ^= WA_STATE_FULLSCREEN;
    }
    // window->state.window.w = window->rect.right;
    // window->state.window.h = window->rect.bottom;
    //wa_resize(window, window->state.window.w, window->state.window.h);
}

void 
wa_window_stop(wa_window_t* window)
{
    window->running = false;
}

void 
wa_window_delete(wa_window_t* window)
{
    UnregisterClass(window->class_name, window->instance);
    free(window);
}

void
wa_window_vsync(wa_window_t* window, bool vsync)
{
    ((BOOL(WINAPI*)(int))wglGetProcAddress("wglSwapIntervalEXT"))(vsync);
    window->state.window.vsync = vsync;
}

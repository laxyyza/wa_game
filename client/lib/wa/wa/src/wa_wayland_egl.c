#include "wa_wayland_egl.h"
#include "wa_wayland.h"
#include "wa_log.h"

#define WA_EGL_ERROR egl_get_err_string(eglGetError())

static const char* 
egl_get_err_string(EGLint error)
{
    switch(error)
    {
        case EGL_SUCCESS: return "No error";
        case EGL_NOT_INITIALIZED: return "EGL not initialized or failed to initialize";
        case EGL_BAD_ACCESS: return "Resource inaccessible";
        case EGL_BAD_ALLOC: return "Cannot allocate resources";
        case EGL_BAD_ATTRIBUTE: return "Unrecognized attribute or attribute value";
        case EGL_BAD_CONTEXT: return "Invalid EGL context";
        case EGL_BAD_CONFIG: return "Invalid EGL frame buffer configuration";
        case EGL_BAD_CURRENT_SURFACE: return "Current surface is no longer valid";
        case EGL_BAD_DISPLAY: return "Invalid EGL display";
        case EGL_BAD_SURFACE: return "Invalid surface";
        case EGL_BAD_MATCH: return "Inconsistent arguments";
        case EGL_BAD_PARAMETER: return "Invalid argument";
        case EGL_BAD_NATIVE_PIXMAP: return "Invalid native pixmap";
        case EGL_BAD_NATIVE_WINDOW: return "Invalid native window";
        case EGL_CONTEXT_LOST: return "Context lost";
        default: return "Unknown";
    }
}

bool 
wa_window_egl_init(wa_window_t* window)
{
    const wa_window_state_t* wstate = &window->state.window;

    const EGLint context_attr[] = {
        EGL_CONTEXT_MAJOR_VERSION, 4,
        EGL_CONTEXT_MINOR_VERSION, 6,

        EGL_CONTEXT_OPENGL_PROFILE_MASK, 
        EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,

        EGL_NONE
    };
    const EGLint config_attr[] = {
        EGL_RED_SIZE,       wstate->egl.red_size,
        EGL_GREEN_SIZE,     wstate->egl.green_size,
        EGL_BLUE_SIZE,      wstate->egl.red_size,
        EGL_ALPHA_SIZE,     wstate->egl.alpha_size,
        EGL_DEPTH_SIZE,     wstate->egl.depth_size,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_NONE
    };
    i32 n;

    if ((window->egl_display = eglGetDisplay(window->wl_display)) == EGL_NO_DISPLAY)
    {
        wa_logf(WA_FATAL, "EGL Get Display: %s\n", WA_EGL_ERROR);
        return false;
    }

    if (!eglInitialize(window->egl_display, 
                       &window->egl_major, &window->egl_minor))
    {
        wa_logf(WA_FATAL, "EGL Initialize: %s\n", WA_EGL_ERROR);
        return false;
    }

    wa_log(WA_VBOSE, "EGL Display: major: %d, minor: %d\n\tVersion: %s\n\tVendor: %s\n\tExtenstions: %s\n", 
            window->egl_major, window->egl_minor, eglQueryString(window->egl_display, EGL_VERSION),
            eglQueryString(window->egl_display, EGL_VENDOR), 
            eglQueryString(window->egl_display, EGL_EXTENSIONS));

    if (!eglBindAPI(EGL_OPENGL_API))
    {
        wa_logf(WA_ERROR, "EGL Bind API: %s\n", WA_EGL_ERROR);
        return false;
    }

    if (!eglChooseConfig(window->egl_display, config_attr, 
                         &window->egl_config, 1, &n) || n != 1)
    {
        wa_logf(WA_ERROR, "EGL Choose Config: %s\n", WA_EGL_ERROR);
        return false;
    }

    if ((window->egl_context = eglCreateContext(window->egl_display, 
                                                window->egl_config, 
                                                EGL_NO_CONTEXT, 
                                                context_attr)) == EGL_NO_CONTEXT)
    {
        wa_logf(WA_ERROR, "EGL Create Context: %s\n", WA_EGL_ERROR);
        return false;
    }

    if ((window->wl_egl_window = wl_egl_window_create(window->wl_surface, 
                                                      window->state.window.w, 
                                                      window->state.window.h)) == EGL_NO_SURFACE)
    {
        wa_logf(WA_ERROR, "wl_egl_window_create(): %s\n", WA_EGL_ERROR);
        return false;
    }

    if ((window->egl_surface = eglCreateWindowSurface(window->egl_display, 
                                                      window->egl_config, 
                                                      (EGLNativeWindowType)window->wl_egl_window, 
                                                      NULL)) == EGL_NO_SURFACE)
    {
        wa_logf(WA_ERROR, "EGL Create Window Surface: %s\n", WA_EGL_ERROR);
        return false;
    }

    if (!eglMakeCurrent(window->egl_display, window->egl_surface, 
                        window->egl_surface, window->egl_context))
    {
        wa_logf(WA_ERROR, "EGL Make current: %s\n", WA_EGL_ERROR);
        return false;
    }

    return true;
}

void 
wa_window_egl_cleanup(wa_window_t* window)
{
    if (window->wl_egl_window)
        wl_egl_window_destroy(window->wl_egl_window);
    if (window->egl_display)
    {
        eglMakeCurrent(window->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (window->egl_surface)
            eglDestroySurface(window->egl_display, window->egl_surface);
        if (window->egl_context)
            eglDestroyContext(window->egl_display, window->egl_context);
        eglTerminate(window->egl_display);
    }
}

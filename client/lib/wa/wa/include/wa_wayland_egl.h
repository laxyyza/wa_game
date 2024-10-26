#ifndef _WA_WAYLAND_EGL_H_
#define _WA_WAYLAND_EGL_H_

#include "wa.h"

bool wa_window_egl_init(wa_window_t* window);
void wa_window_egl_cleanup(wa_window_t* window);

#endif // _WA_WAYLAND_EGL_H_

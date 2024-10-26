#ifndef _WAAPP_H_
#define _WAAPP_H_

#include <wa.h>
#include <stdbool.h>
#include "renderer.h"
#include "vec.h"
#include "util.h"

struct nk_wa;
struct nk_conext;

typedef struct 
{
    wa_window_t* window;
    vec4f_t bg_color;
    vec4f_t color;
    ren_t ren;

    array_t rects;
    vec2f_t mouse;
    vec2f_t mouse_prev;
    vec3f_t cam;

    vec2f_t dir;

    struct nk_wa* nk_wa;
    struct nk_context* nk_ctx;

    bool do_rotation;
} waapp_t;

i32 waapp_init(waapp_t* app, i32 argc, const char** argv);
void waapp_run(waapp_t* app);
void waapp_cleanup(waapp_t* app);

#endif // _WAAPP_H_

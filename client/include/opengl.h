#ifndef _WA_OPENGL_H_
#define _WA_OPENGL_H_

#include "app.h"
#include <glad/glad.h>
#include <assert.h>

bool waapp_opengl_init(waapp_t* app);
void waapp_opengl_cleanup(waapp_t* app);
void waapp_opengl_draw(waapp_t* app);
void waapp_render_map(waapp_t* app, cg_map_t* map);
u64  gl_sizeof(u32 type);

#endif // _WA_OPENGL_H_

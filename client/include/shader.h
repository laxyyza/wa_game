#ifndef _SHADER_H_
#define _SHADER_H_

#include "int.h"
#include "vec.h"
#include "mat.h"
#include <ght.h>

typedef struct 
{
    u32 id;
    ght_t location_cache;
} shader_t;

void shader_init(shader_t* shader, const char* vertex_path, const char* fragment_path);
void shader_del(shader_t* shader);
void shader_bind(const shader_t* shader);
void shader_unbind(void);
void shader_uniform4f(shader_t* shader, 
                      const char* name, 
                      f32 x, f32 y, f32 z, f32 w);
void shader_uniform_vec4f(shader_t* shader,
                          const char* name,
                          const vec4f_t* vec4);
void shader_uniform1f(shader_t* shader, const char* name, f32 x);
void 
shader_uniform_vec2f(shader_t* shader,
                     const char* name,
                     const vec2f_t* vec);
void shader_uniform1i(shader_t* shader, 
                      const char* name, 
                      i32 value);

void shader_uniform1iv(shader_t* shader, 
                        const char* name, 
                        i32* value,
                        u32 count);
void shader_uniform_mat4f(shader_t* shader, 
                      const char* name, 
                      const mat4_t* mat4);
u32 shader_compile(const char* path, u32 type);
 
#endif // _SHADER_H_

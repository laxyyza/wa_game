#ifndef _VEC_H_
#define _VEC_H_

#include "int.h"

typedef struct 
{
    f32 x;
    f32 y;
} vec2f_t;

typedef struct 
{
    f32 x;
    f32 y;
    f32 z;
} vec3f_t;

typedef struct 
{
    f32 x;
    f32 y;
    f32 z;
    f32 w;
} vec4f_t;

typedef struct 
{
    i32 x;
    i32 y;
} vec2i_t;

ALWAYS_INLINE inline vec2f_t
vec2f(f32 x, f32 y)
{
    vec2f_t vec = {x, y};
    return vec;
}

ALWAYS_INLINE inline vec3f_t
vec3f(f32 x, f32 y, f32 z)
{
    vec3f_t vec = {x, y, z};
    return vec;
}

ALWAYS_INLINE inline vec4f_t
vec4f(f32 x, f32 y, f32 z, f32 w)
{
    vec4f_t vec = {x, y, z, w};
    return vec;
}

ALWAYS_INLINE inline vec4f_t
rgba(u32 color)
{
    vec4f_t vec = {
        (color >> 24 & 0xFF) / 255.0,
        (color >> 16 & 0xFF) / 255.0,
        (color >> 8  & 0xFF) / 255.0,
        (color       & 0xFF) / 255.0
    };
    return vec;
}

#endif // _VEC_H_

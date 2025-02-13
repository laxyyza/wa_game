#ifndef _VEC_H_
#define _VEC_H_

#include "int.h"
#include <math.h>

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

typedef struct 
{
    u16 x;
    u16 y;
} vec2u16_t;

ALWAYS_INLINE vec2f_t
vec2f(f32 x, f32 y)
{
    vec2f_t vec = {x, y};
    return vec;
}

ALWAYS_INLINE vec3f_t
vec3f(f32 x, f32 y, f32 z)
{
    vec3f_t vec = {x, y, z};
    return vec;
}

ALWAYS_INLINE vec4f_t
vec4f(f32 x, f32 y, f32 z, f32 w)
{
    vec4f_t vec = {x, y, z, w};
    return vec;
}

ALWAYS_INLINE vec4f_t
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

ALWAYS_INLINE void
vec2f_norm(vec2f_t* vec)
{
	const f32 manitude = sqrtf(vec->x * vec->x + vec->y * vec->y);


	if (manitude != 0)
	{
		vec->x /= manitude;
		vec->y /= manitude;
	}
}

#endif // _VEC_H_

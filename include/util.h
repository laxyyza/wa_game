#ifndef _WAAPP_UTIL_H_
#define _WAAPP_UTIL_H_

#include "int.h"
#include "mat.h"
#include "vec.h"
#include "rect.h"
#include <math.h>

ALWAYS_INLINE f32 
deg_to_rad(f32 degress)
{
    return degress * (M_PI / 180);
}

ALWAYS_INLINE f32
rad_to_deg(f32 radians)
{
    return radians * (180 / M_PI);
}

ALWAYS_INLINE f32
angle(const vec2f_t* a, const vec2f_t* b)
{
    return atan2(b->y - a->y, b->x - a->x);
}

ALWAYS_INLINE f32
angle_deg(const vec2f_t* a, const vec2f_t* b)
{
    return rad_to_deg(angle(a, b));
}

ALWAYS_INLINE vec2f_t
rect_origin(const rect_t* rect)
{
    return vec2f(
        rect->pos.x + rect->origin.x, 
        rect->pos.y + rect->origin.y
    );
}

void print_mat4(const char* name, const mat4_t* m);
void print_vec2f(const char* name, const vec2f_t* v);
void print_vec3f(const char* name, const vec3f_t* v);
void print_vec4f(const char* name, const vec4f_t* v);

#endif // _WAAPP_UTIL_H_

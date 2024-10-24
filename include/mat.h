#ifndef _MAT_H_
#define _MAT_H_

#include "int.h"
#include "vec.h"
#include <stdio.h>
#include <string.h>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

typedef struct 
{
    f32 m[4][4];
} mat4_t;

ALWAYS_INLINE mat4_t 
mat_ortho(f32 left, f32 right, f32 bot, f32 top, f32 znear, f32 zfar)
{
    mat4_t ret = {0};

    ret.m[0][0] = 2.0f / (right - left);
    ret.m[1][1] = 2.0f / (top - bot);
    ret.m[2][2] = -2.0f / (zfar - znear);
    ret.m[3][0] = -(right + left) / (right - left);
    ret.m[3][1] = -(top + bot) / (top - bot);
    ret.m[3][2] = -(zfar + znear) / (zfar - znear);
    ret.m[3][3] = 1.0f;

    return ret;
}

ALWAYS_INLINE void
mat4_identity(mat4_t* mat)
{
    for (u32 i = 0; i < 4; i++)
    {
        for (u32 j = 0; j < 4; j++)
        {
            if (j == i)
                mat->m[i][j] = 1.0;
            else 
                mat->m[i][j] = 0.0;
        }
    }
}

ALWAYS_INLINE void
mat4_translate(mat4_t* mat, const vec3f_t* offset)
{
    mat4_t trans;
    mat4_identity(&trans);

    trans.m[0][3] = offset->x;
    trans.m[1][3] = offset->y;
    trans.m[2][3] = offset->z;

    mat4_t ret;

    for (u32 i = 0; i < 4; i++)
    {
        for (u32 j = 0; j < 4; j++)
        {
            ret.m[i][j] = 0.0;
            for (u32 k = 0; k < 4; k++)
                ret.m[i][j] += mat->m[i][k] * trans.m[j][k];
        }
    }

    memcpy(mat, &ret, sizeof(mat4_t));
}

ALWAYS_INLINE void
mat4_mul(mat4_t* ret, const mat4_t* a, const mat4_t* b)
{
    mat4_t tmp;
    for (u32 i = 0; i < 4; i++)
    {
        for (u32 j = 0; j < 4; j++)
        {
            tmp.m[i][j] = 0.0f;
            for (u32 k = 0; k < 4; k++)
                tmp.m[i][j] += a->m[i][k] * b->m[k][j];
        }
    }

    memcpy(ret, &tmp, sizeof(mat4_t));
}

ALWAYS_INLINE void
mat4_mul_f32(mat4_t* ret, f32 scalar)
{
    for (u32 i = 0; i < 4; i++)
        for (u32 j = 0; j < 4; j++)
            ret->m[i][j] *= scalar;
}

ALWAYS_INLINE void
mat4_mul_vec4f(vec4f_t* ret, const mat4_t* m, const vec4f_t* v)
{
    f32* ret_array = (f32*)ret;
    const f32* v_array = (const f32*)v;
    for (u32 i = 0; i < 4; i++)
    {
        ret_array[i] = m->m[i][0] * v_array[0] + 
                       m->m[i][1] * v_array[1] +
                       m->m[i][2] * v_array[2] +
                       m->m[i][3] * v_array[3];
    }
}

ALWAYS_INLINE void
mat4_add_vec4f(vec4f_t* ret, const mat4_t* m, const vec4f_t* v)
{
    f32* ret_array = (f32*)ret;
    const f32* v_array = (const f32*)v;
    for (u32 i = 0; i < 4; i++)
    {
        ret_array[i] = m->m[i][0] + v_array[0] + 
                       m->m[i][1] + v_array[1] +
                       m->m[i][2] + v_array[2] +
                       m->m[i][3] + v_array[3];
    }
}

ALWAYS_INLINE void
mat4_scale(mat4_t* mat, f32 x, f32 y, f32 z)
{
    mat4_t scaling;
    mat4_identity(&scaling);

    scaling.m[0][0] = x;
    scaling.m[1][1] = y;
    scaling.m[2][2] = z;

    mat4_t ret;
    for (u32 i = 0; i < 4; i++)
    {
        for (u32 j = 0; j < 4; j++)
        {
            ret.m[i][j] = 0.0f;
            for (u32 k = 0; k < 4; k++)
                ret.m[i][j] += mat->m[i][k] * scaling.m[k][j];
        }
    }
    for (int i = 0; i < 4; i++) 
        for (int j = 0; j < 4; j++) 
            mat->m[i][j] = ret.m[i][j];
}

ALWAYS_INLINE void
mat4_scale_vec2f(mat4_t* ret, const vec2f_t* v)
{
    mat4_identity(ret);

    ret->m[0][0] = v->x;
    ret->m[1][1] = v->y;
}

void mat4_rotate(mat4_t* ret, const mat4_t* m, float angle);
void mat4_print(const mat4_t* mat);

#endif // _MAT_H_

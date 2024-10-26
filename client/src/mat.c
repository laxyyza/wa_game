#include "mat.h"
#include <math.h>
#include <string.h>

void 
mat4_rotate(mat4_t* ret, const mat4_t* m, f32 angle)
{
    mat4_t tmp;
    const f32 cos_theta = cosf(angle);
    const f32 sin_theta = sinf(angle);

    const mat4_t rotation = {{
        {cos_theta, -sin_theta, 0.0, 0.0},
        {sin_theta,  cos_theta, 0.0, 0.0},
        {0.0, 0.0, 1.0, 0.0},
        {0.0, 0.0, 0.0, 1.0},
    }};

    for (u32 i = 0; i < 4; i++)
    {
        for (u32 j = 0; j < 4; j++)
        {
            tmp.m[i][j] = rotation.m[i][0] * m->m[0][j] + 
                          rotation.m[i][1] * m->m[1][j] +
                          rotation.m[i][2] * m->m[2][j] +
                          rotation.m[i][3] * m->m[3][j];
        }
    }
    memcpy(ret, &tmp, sizeof(mat4_t));
}

void
mat4_print(const mat4_t* mat)
{
    for (u32 i = 0; i < 4; i++)
    {
        printf("{ ");
        for (u32 j = 0; j < 4; j++)
        {
            printf("%.2f, ", mat->m[i][j]);
        }
        printf("}\n");
    }
}

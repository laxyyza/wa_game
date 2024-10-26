#include "util.h"

void
print_mat4(const char* name, const mat4_t* m)
{
    printf("mat4_t %s:\n", name);
    for (u32 i = 0; i < 4; i++)
    {
        printf("\t");
        for (u32 j = 0; j < 4; j++)
            printf("%.2f\t", m->m[i][j]);
        printf("\n");
    }
}

void
print_vec2f(const char* name, 
            const vec2f_t* v)
{
    printf("vec2f_t %s: {%f, %f}\n",
           name, v->x, v->y);
}

void
print_vec3f(const char* name, 
            const vec3f_t* v)
{
    printf("vec3f_t %s: {%f, %f, %f}\n",
           name, v->x, v->y, v->z);
}

void
print_vec4f(const char* name, 
            const vec4f_t* v)
{
    printf("vec4f_t %s: {%f, %f, %f, %f}\n",
           name, v->x, v->y, v->z, v->w);
}

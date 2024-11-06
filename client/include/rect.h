#ifndef _RECT_H_
#define _RECT_H_

#include "texture.h"
#include "vec.h"

#define RECT_VERT 4
#define RECT_SIZE sizeof(vertex_t) * RECT_VERT

typedef struct 
{
    vec2f_t pos;
    vec2f_t origin;
    vec2f_t size;
    vec4f_t color;
    texture_t* texture;
    f32 rotation; // radians
	void* render_data;
} rect_t;

void rect_init(rect_t* rect, vec2f_t pos, vec2f_t size, u32 color, texture_t* texture);

#endif // _RECT_H_

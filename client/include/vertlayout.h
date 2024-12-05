#ifndef _VERTEX_BUFFER_LAYOUT_H_
#define _VERTEX_BUFFER_LAYOUT_H_

#include "array.h"

typedef struct 
{
    u32 count;
    u32 type;
    bool norm;
} vertbuf_element_t;

typedef struct 
{
    array_t elements;
    u32 stride;
} vertlayout_t;

void vertlayout_init(vertlayout_t* layout);
void vertlayout_del(vertlayout_t* layout);
void vertlayout_add_f32(vertlayout_t* layout, u32 count);
void vertlayout_add_u32(vertlayout_t* layout, u32 count);
void vertlayout_add_i32(vertlayout_t* layout, u32 count);

#endif // _VERTEX_BUFFER_LAYOUT_H_

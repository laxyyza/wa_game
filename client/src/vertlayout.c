#include "vertlayout.h"
#include "array.h"
#include "opengl.h"

void 
vertlayout_init(vertlayout_t* layout)
{
    array_init(&layout->elements, sizeof(vertbuf_element_t), 10);
    layout->stride = 0;
}

void 
vertlayout_del(vertlayout_t* layout)
{
    array_del(&layout->elements);
}

void 
vertlayout_add_f32(vertlayout_t* layout, u32 count)
{
    vertbuf_element_t* element = array_add_into(&layout->elements);
    element->type = GL_FLOAT;
    element->count = count;
    element->norm = false;
    layout->stride += gl_sizeof(element->type) * count;
}

void 
vertlayout_add_u32(vertlayout_t* layout, u32 count)
{
    vertbuf_element_t* element = array_add_into(&layout->elements);
    element->type = GL_UNSIGNED_INT;
    element->count = count;
    element->norm = false;
    layout->stride += gl_sizeof(element->type) * count;
}

void 
vertlayout_add_i32(vertlayout_t* layout, u32 count)
{
    vertbuf_element_t* element = array_add_into(&layout->elements);
    element->type = GL_INT;
    element->count = count;
    element->norm = false;
    layout->stride += gl_sizeof(element->type) * count;
}

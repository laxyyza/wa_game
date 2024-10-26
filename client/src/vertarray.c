#include "vertarray.h"
#include "vertbuf.h"
#include "opengl.h"
#include "vertlayout.h"

void 
vertarray_init(vertarray_t* va)
{
    glGenVertexArrays(1, &va->id);
    vertarray_bind(va);
}

void 
vertarray_del(vertarray_t* va)
{
    glDeleteVertexArrays(1, &va->id);
}

void 
vertarray_bind(const vertarray_t* va)
{
    glBindVertexArray(va->id);
}

void
vertarray_unbind(void)
{
    glBindVertexArray(0);
}

void 
vertarray_add(vertarray_t* va, const vertbuf_t* vb, const vertlayout_t* layout)
{
    vertarray_bind(va);
    vertbuf_bind(vb);
    const vertbuf_element_t* elements = (vertbuf_element_t*)layout->elements.buf;
    u64 offset = 0;
    for (u32 i = 0; i < layout->elements.count; i++)
    {
        const vertbuf_element_t* ele = elements + i;
        glEnableVertexAttribArray(i);
        glVertexAttribPointer(i, ele->count, ele->type, ele->norm, 
                              layout->stride, (const void*)offset);
        offset += gl_sizeof(ele->type) * ele->count;
    }
}


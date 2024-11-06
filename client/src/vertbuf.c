#include "vertbuf.h"
#include "opengl.h"
#include "renderer.h"

void 
vertbuf_init(vertbuf_t* vb, const void* data, u32 max_count, u32 vertex_size)
{
    glGenBuffers(1, &vb->id);
    vertbuf_bind(vb);
    vb->size = max_count * vertex_size;
    glBufferData(GL_ARRAY_BUFFER, vb->size, data, GL_DYNAMIC_DRAW);

    vb->buf = calloc(1, vb->size);
    vb->count = 0;
    vb->max_count = max_count;
	vb->vertex_size = vertex_size;
    // glBufferStorage(GL_ARRAY_BUFFER, size, data, 
    //                 GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
}

void 
vertbuf_del(vertbuf_t* vb)
{
    glDeleteBuffers(1, &vb->id);
    free(vb->buf);
}

void 
vertbuf_bind(const vertbuf_t* vertbuf)
{
    glBindBuffer(GL_ARRAY_BUFFER, vertbuf->id);
}

void
vertbuf_submit(const vertbuf_t* vb)
{
    glBufferSubData(GL_ARRAY_BUFFER,
                    0,
                    vb->count * vb->vertex_size,
                    vb->buf);
}

void vertbuf_unbind(void)
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void* 
vertbuf_map(const vertbuf_t* vb)
{
    vertbuf_bind(vb);
    return glMapBufferRange(GL_ARRAY_BUFFER, 0, vb->size, 
                            GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
}

void
vertbuf_unmap(const vertbuf_t* vb)
{
    vertbuf_bind(vb);
    glUnmapBuffer(GL_ARRAY_BUFFER);
}

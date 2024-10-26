#ifndef _VERTEX_BUFFER_H_
#define _VERTEX_BUFFER_H_

#include "int.h"

typedef struct vertex vertex_t;

typedef struct 
{
    u32 id;
    u32 size;
    u32 max_count;
    u32 count;
    vertex_t* buf;
} vertbuf_t;

void vertbuf_init(vertbuf_t* vertbuf, const void* data, u32 max_count);
void vertbuf_del(vertbuf_t* vertbuf);
void vertbuf_bind(const vertbuf_t* vertbuf);
void vertbuf_unbind(void);
void vertbuf_submit(const vertbuf_t* vb);
void* vertbuf_map(const vertbuf_t* buf);
void vertbuf_unmap(const vertbuf_t* vb);

#endif // _VERTEX_BUFFER_H_

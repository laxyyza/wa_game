#ifndef _VERTEX_ARRAY_H_
#define _VERTEX_ARRAY_H_

#include "vertbuf.h"
#include "vertlayout.h"

typedef struct
{
    u32 id;
} vertarray_t;

void vertarray_init(vertarray_t* vertarray);
void vertarray_del(vertarray_t* vertarray);
void vertarray_bind(const vertarray_t* vertarray);
void vertarray_unbind(void);
void vertarray_add(vertarray_t* va, const vertbuf_t* vb, const vertlayout_t* layout);

#endif // _VERTEX_ARRAY_H_

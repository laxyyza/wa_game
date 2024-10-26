#ifndef _INDEX_BUFFER_H_
#define _INDEX_BUFFER_H_

#include "int.h"
#include "array.h"

enum idxbuf_type 
{
    IDXBUF_UINT8,
    IDXBUF_UINT16,
    IDXBUF_UINT32,
};

typedef struct 
{
    u32 id;
    u32 type;
    u32 size;
    array_t array;
} idxbuf_t;

void idxbuf_init(idxbuf_t* idxbuf, enum idxbuf_type type, const void* data, u32 count);
void idxbuf_del(idxbuf_t* idxbuf);
void idxbuf_bind(const idxbuf_t* idxbuf);
void idxbuf_submit(const idxbuf_t* ib);
void idxbuf_unbind(void);
void idxbuf_memcpy(const idxbuf_t* ib, const void* data, u64 size);

#endif // _INDEX_BUFFER_H_

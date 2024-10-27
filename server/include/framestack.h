#ifndef _SERVER_FRAMESTACK_H_
#define _SERVER_FRAMESTACK_H_

#include "array.h"

typedef struct 
{
	void*	buf;
	u64		size;
	u64		idx;
} memframe_t;

typedef struct 
{
	array_t frames;
} framestack_t;

void  framestack_init(framestack_t* fs);
void* framestack_alloc(framestack_t* fs, u64 size);
void  framestack_clear(framestack_t* fs);
void  framestack_free(framestack_t* fs);

#endif // _SERVER_FRAMESTACK_H_

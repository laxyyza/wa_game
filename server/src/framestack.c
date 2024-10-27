#include "framestack.h"
#include <stdlib.h>
#include <stdio.h>

#define FRAME_SIZE 4096

static void
memframe_init(memframe_t* frame, u64 size)
{
	frame->size = (size == 0) ? FRAME_SIZE : size;
	frame->buf = malloc(frame->size);
	frame->idx = 0;
}

void  
framestack_init(framestack_t* fs)
{
	array_init(&fs->frames, sizeof(memframe_t), 1);
	memframe_t* mf = array_add_into(&fs->frames);
	memframe_init(mf, 0);
}

static void*
framestack_push(memframe_t* frame, u64 size)
{
	void* ret = NULL;
	if (frame->idx + size < frame->size)
	{
		ret = frame->buf + frame->idx;
		frame->idx += size;
	}
	return ret;
}

void* 
framestack_alloc(framestack_t* fs, u64 size)
{
	void* ret;
	memframe_t* frames = (memframe_t*)fs->frames.buf;
	memframe_t* frame;

	for (u32 i = 0; i < fs->frames.count; i++)
	{
		frame = frames + i;
		if ((ret = framestack_push(frame, size)))
			return ret;
	}
	frame = array_add_into(&fs->frames);
	memframe_init(frame, (size > FRAME_SIZE) ? size : 0);
	ret = framestack_push(frame, size);
	return ret;
}

void  
framestack_clear(framestack_t* fs)
{
	memframe_t* frames = (memframe_t*)fs->frames.buf;
	memframe_t* frame;

	for (u32 i = 1; i < fs->frames.count; i++)
	{
		frame = frames + i;
		free(frame->buf);
	}
	array_clear(&fs->frames, true);
	fs->frames.count = 1;
	frames->idx = 0;
}

void  
framestack_free(framestack_t* fs)
{
	memframe_t* frames = (memframe_t*)fs->frames.buf;
	memframe_t* frame;

	for (u32 i = 0; i < fs->frames.count; i++)
	{
		frame = frames + i;
		free(frame->buf);
	}
	array_del(&fs->frames);
}

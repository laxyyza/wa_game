#ifndef _FRAMEBUFFER_H_
#define _FRAMEBUFFER_H_

#include "int.h"
#include "texture.h"

typedef struct renderer ren_t;

typedef struct
{
	u32 id;
	texture_t texture;
	i32 w;
	i32 h;
} framebuffer_t, fbo_t;

void framebuffer_init(fbo_t* fbo, i32 w, i32 h);
void framebuffer_bind(const fbo_t* fbo);
void framebuffer_unbind(ren_t* ren);

#endif // _FRAMEBUFFER_H_

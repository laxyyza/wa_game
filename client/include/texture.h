#ifndef _TEXTURE_H_
#define _TEXTURE_H_

#include "int.h"

enum filter 
{	
	TEXTURE_LINEAR,
	TEXTURE_NEAREST,
};

typedef struct 
{
    u32 id;
    u8* buf;
    i32 w;
    i32 h;
    i32 bpp;    // Bits Per Pixel
	const char* name;
} texture_t;

texture_t* texture_load(const char* filename, enum filter);
void texture_init(texture_t* text, const char* filename, enum filter);
void texture_del(texture_t* text);

void texture_bind(const texture_t* text, u32 slot);
void texture_unbind(void);

#endif // _TEXTURE_H_

#ifndef _TEXTURE_H_
#define _TEXTURE_H_

#include "int.h"

typedef struct 
{
    u32 id;
    u8* buf;
    i32 w;
    i32 h;
    i32 bpp;    // Bits Per Pixel
} texture_t;

texture_t* texture_load(const char* filename);
void texture_init(texture_t* text, const char* filename);
void texture_del(texture_t* text);

void texture_bind(const texture_t* text, u32 slot);
void texture_unbind(void);

#endif // _TEXTURE_H_

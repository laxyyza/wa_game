#ifndef _LOADING_H_
#define _LOADING_H_

#include "texture.h"

texture_t** waapp_load_textures_threaded(const char* dir, u32* count);

#endif // _LOADING_H_

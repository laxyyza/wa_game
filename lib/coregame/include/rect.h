#ifndef _COREGAME_RECT_H_
#define _COREGAME_RECT_H_

#include "vec.h"

typedef struct 
{
	vec2f_t pos;
	vec2f_t size;
} cg_rect_t;

ALWAYS_INLINE cg_rect_t 
cg_rect(vec2f_t pos, vec2f_t size)
{
	cg_rect_t rect = {
		pos,
		size
	};
	return rect;
}

#endif // _COREGAME_RECT_H_

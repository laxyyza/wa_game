#include "rect.h"

void
rect_init(rect_t* rect, vec2f_t pos, vec2f_t size, u32 color, texture_t* texture)
{
    rect->pos = pos;
    rect->size = size;
    rect->color = rgba(color);
    rect->texture = texture;
    rect->origin.x = rect->size.x / 2;
    rect->origin.y = rect->size.y / 2;
}


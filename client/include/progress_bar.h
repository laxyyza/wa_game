#ifndef _CLIENT_PROGRESS_BAR_H_
#define _CLIENT_PROGRESS_BAR_H_

#include "rect.h"

typedef struct 
{	
	const vec2f_t* parent_pos;
	vec2f_t offset;

	rect_t background;
	rect_t fill;
	f32 fill_width;
	const f32* max;
	const f32* val;
} progress_bar_t;

void progress_bar_init(progress_bar_t* bp, 
					   vec2f_t pos, 
					   vec2f_t size, 
					   const f32* max, 
					   const f32* val,
					   u32 color);
void progress_bar_set_pos(progress_bar_t* bp, vec2f_t new_pos);
void progress_bar_update_pos(progress_bar_t* bp);
void progress_bar_update(progress_bar_t* bp);
void progress_bar_update_valmax(progress_bar_t* bp, f32 val, f32 max);

#endif // _CLIENT_PROGRESS_BAR_H_

#include "progress_bar.h"
#include "app.h"

#define BACKGROUND_COLOR 0x000000AA

void 
progress_bar_init(progress_bar_t* bp, vec2f_t pos, vec2f_t size, 
					   const f32* max, const f32* val, u32 color)
{
	bp->max = max;
	bp->val = val;

	rect_init(&bp->background, pos, size, BACKGROUND_COLOR, NULL);
	rect_init(&bp->fill, pos, size, color, NULL);
	bp->fill_width = bp->fill.size.x;
}

void 
progress_bar_set_pos(progress_bar_t* bp, vec2f_t new_pos)
{
	bp->background.pos = new_pos;
	bp->fill.pos = new_pos;
}

void 
progress_bar_update_pos(progress_bar_t* bp)
{
	if (bp->parent_pos == NULL)
		return;

	progress_bar_set_pos(bp, 
		vec2f(
			bp->parent_pos->x + bp->offset.x,
			bp->parent_pos->y + bp->offset.y
		)
	);
}

void 
progress_bar_update(progress_bar_t* bp)
{
	progress_bar_update_valmax(bp, *bp->val, *bp->max);
}

void 
progress_bar_update_valmax(progress_bar_t* bp, f32 val, f32 max)
{
	f32 per = (val / max) * 100.0;

	f32 bar_fill = (per * bp->fill_width) / 100.0;
	bp->fill.size.x = bar_fill;
}

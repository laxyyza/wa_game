#ifndef _GUI_GUI_H_
#define _GUI_GUI_H_

#include "app.h"

void gui_init(waapp_t* app);
void gui_new_frame(waapp_t* app);
void gui_scroll(waapp_t* app, f32 xoff, f32 yoff);
void gui_render(waapp_t* app);
void gui_set_font(waapp_t* app, const struct nk_font* font);

#endif // _GUI_GUI_H_

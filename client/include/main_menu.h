#ifndef _WAAPP_MAIN_MENU_H_
#define _WAAPP_MAIN_MENU_H_

#include "wa.h"

typedef struct waapp waapp_t;

typedef struct 
{

} waapp_main_menu_t;

void main_menu_init(waapp_t* app, void* data);
void main_menu_update(waapp_t* app, void* data);
i32 main_menu_event(waapp_t* app, const wa_event_t* ev);
void main_menu_exit(waapp_t* app, void* data);

#endif // _WAAPP_MAIN_MENU_H_

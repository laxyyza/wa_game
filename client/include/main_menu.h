#ifndef _WAAPP_MAIN_MENU_H_
#define _WAAPP_MAIN_MENU_H_

#include "wa.h"
#include "netdef.h"

typedef struct waapp waapp_t;

#define MM_STATE_STRING_MAX 128

typedef struct 
{
	char ipaddr[INET6_ADDRSTRLEN];
	char username[PLAYER_NAME_MAX];
} main_menu_save_data_t;

typedef struct 
{
	main_menu_save_data_t sd;
	char state[MM_STATE_STRING_MAX];
} waapp_main_menu_t;

void* main_menu_init(waapp_t* app);
void main_menu_enter(waapp_t* app, waapp_main_menu_t* mm);
void main_menu_update(waapp_t* app, waapp_main_menu_t* mm);
i32 main_menu_event(waapp_t* app, const wa_event_t* ev);
void main_menu_exit(waapp_t* app, waapp_main_menu_t* mm);
void main_menu_cleanup(waapp_t* app, waapp_main_menu_t* mm);

#endif // _WAAPP_MAIN_MENU_H_

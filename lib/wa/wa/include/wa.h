#ifndef _WA_H_
#define _WA_H_

#include "wa_int.h"
#include "wa_event.h"
#include "wa_keys.h"

#define _WA_UNUSED __attribute__((unused))

typedef struct wa_window wa_window_t;

typedef struct 
{
    void (*event)(wa_window_t* window, const wa_event_t* event, void* data);
    void (*update)(wa_window_t* window, void* data);
    void (*draw)(wa_window_t* window, void* data);
    void (*close)(wa_window_t* window, void* data);
    void (*focus)(wa_window_t* window, void* data);
    void (*unfocus)(wa_window_t* window, void* data);
} wa_callbacks_t;

#define WA_STATE_FULLSCREEN 0x01
#define WA_STATE_MAXIMIZED  0x02
#define WA_STATE_ACTIVE     0x04
#define WA_STATE_SUSPENDED  0x08

typedef struct 
{
    i32 w;
    i32 h;
    u16 state;
    const char* title;
    bool vsync;

    struct { /* Wayland specific */
        const char* app_id;
        bool opaque;
    } wayland;

    struct {
        u8 red_size;
        u8 green_size;
        u8 blue_size;
        u8 alpha_size;
        u8 depth_size;
    } egl;
} wa_window_state_t;

typedef struct 
{
    wa_window_state_t   window;
    wa_callbacks_t      callbacks;
    void*               user_data;
    u8             key_map[WA_KEY_LEN];
    u8             mouse_map[WA_MOUSE_LEN];
} wa_state_t;

wa_window_t*    wa_window_create(const char* title, i32 w, int h, bool fullscrenn);
wa_window_t*    wa_window_create_from_state(wa_state_t* state);
void            wa_state_set_default(wa_state_t* state);
void            wa_window_set_callbacks(wa_window_t* window, const wa_callbacks_t* callbacks);
wa_state_t*     wa_window_get_state(wa_window_t* window);
i32             wa_window_mainloop(wa_window_t* window);
void            wa_window_set_fullscreen(wa_window_t* window, bool fullscreen);
void            wa_window_stop(wa_window_t* window);
void            wa_window_delete(wa_window_t* window);
void            wa_window_vsync(wa_window_t* window, bool vsync);

void wa_window_poll_timeout(wa_window_t* window, i32 timeout);
void wa_window_poll(wa_window_t* window);

#endif // _WA_H_

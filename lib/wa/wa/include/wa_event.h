#ifndef _WA_EVENT_H_
#define _WA_EVENT_H_

#include <stdint.h>
#include <stdbool.h>
#include "wa_keys.h"

#define WA_KEY_NAME_LEN 32

enum wa_event_type
{
    WA_EVENT_KEYBOARD,
    WA_EVENT_POINTER,
    WA_EVENT_RESIZE,
    WA_EVENT_MOUSE_BUTTON,
    WA_EVENT_MOUSE_WHEEL,
};

typedef struct  
{
    enum wa_event_type type;
    union {
        struct {
            wa_key_t key;
            bool pressed;
        } keyboard;
        struct {
            int x;
            int y;
        } pointer;
        struct {
            int w;
            int h;
            int old_w;
            int old_h;
        } resize;
        struct {
            wa_mouse_butt_t button;
            bool pressed;
        } mouse;
        struct  {
            int32_t value;
            int32_t axis;
        } wheel;
    };
} wa_event_t;

#endif // _WA_EVENT_H_

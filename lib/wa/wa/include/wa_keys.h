#ifndef _WA_KEYS_H_
#define _WA_KEYS_H_

typedef enum wa_key
{
    WA_KEY_NONE = 0,
    WA_KEY_Q,
    WA_KEY_W,
    WA_KEY_E,
    WA_KEY_R,
    WA_KEY_T,
    WA_KEY_Y,
    WA_KEY_U,
    WA_KEY_I,
    WA_KEY_O,
    WA_KEY_P,
    WA_KEY_A,
    WA_KEY_S,
    WA_KEY_D,
    WA_KEY_F,
    WA_KEY_G,
    WA_KEY_H,
    WA_KEY_J,
    WA_KEY_K,
    WA_KEY_L,
    WA_KEY_Z,
    WA_KEY_X,
    WA_KEY_C,
    WA_KEY_V,
    WA_KEY_B,
    WA_KEY_N,
    WA_KEY_M,

    WA_KEY_0,
    WA_KEY_1,
    WA_KEY_2,
    WA_KEY_3,
    WA_KEY_4,
    WA_KEY_5,
    WA_KEY_6,
    WA_KEY_7,
    WA_KEY_8,
    WA_KEY_9,

    WA_KEY_ESC,
    WA_KEY_TAB,
    WA_KEY_CAPS,
    WA_KEY_LSHIFT,
    WA_KEY_LCTRL,
    WA_KEY_LALT,
    WA_KEY_SPACE,
    WA_KEY_RALT,
    WA_KEY_RSHIFT,
    WA_KEY_ENTER,

    WA_KEY_UP,
    WA_KEY_LEFT,
    WA_KEY_RIGHT,
    WA_KEY_DOWN,

    WA_KEY_LEN
} wa_key_t;

typedef enum wa_mouse_button
{
    WA_MOUSE_UNKOWN = 0,
    WA_MOUSE_LEFT,
    WA_MOUSE_RIGHT,
    WA_MOUSE_MIDDLE,

    WA_MOUSE_LEN
} wa_mouse_butt_t;

#endif

#include "wa_wayland_xkbcommon.h"
#include "wa_wayland.h"
#include <sys/mman.h>

wa_key_t
wa_xkb_to_wa_key(xkb_keysym_t keysym)
{
    switch (keysym)
    {
        case XKB_KEY_Q:
        case XKB_KEY_q:
            return WA_KEY_Q;
        case XKB_KEY_W:
        case XKB_KEY_w:
            return WA_KEY_W;
        case XKB_KEY_E:
        case XKB_KEY_e:
            return WA_KEY_E;
        case XKB_KEY_R:
        case XKB_KEY_r:
            return WA_KEY_R;
        case XKB_KEY_T:
        case XKB_KEY_t:
            return WA_KEY_T;
        case XKB_KEY_Y:
        case XKB_KEY_y:
            return WA_KEY_Y;
        case XKB_KEY_U:
        case XKB_KEY_u:
            return WA_KEY_U;
        case XKB_KEY_I:
        case XKB_KEY_i:
            return WA_KEY_I;
        case XKB_KEY_O:
        case XKB_KEY_o:
            return WA_KEY_O;
        case XKB_KEY_P:
        case XKB_KEY_p:
            return WA_KEY_P;
        case XKB_KEY_A:
        case XKB_KEY_a:
            return WA_KEY_A;
        case XKB_KEY_S:
        case XKB_KEY_s:
            return WA_KEY_S;
        case XKB_KEY_D:
        case XKB_KEY_d:
            return WA_KEY_D;
        case XKB_KEY_F:
        case XKB_KEY_f:
            return WA_KEY_F;
        case XKB_KEY_G:
        case XKB_KEY_g:
            return WA_KEY_G;
        case XKB_KEY_H:
        case XKB_KEY_h:
            return WA_KEY_H;
        case XKB_KEY_J:
        case XKB_KEY_j:
            return WA_KEY_J;
        case XKB_KEY_K:
        case XKB_KEY_k:
            return WA_KEY_K;
        case XKB_KEY_L:
        case XKB_KEY_l:
            return WA_KEY_L;
        case XKB_KEY_Z:
        case XKB_KEY_z:
            return WA_KEY_Z;
        case XKB_KEY_X:
        case XKB_KEY_x:
            return WA_KEY_X;
        case XKB_KEY_C:
        case XKB_KEY_c:
            return WA_KEY_C;
        case XKB_KEY_V:
        case XKB_KEY_v:
            return WA_KEY_V;
        case XKB_KEY_B:
        case XKB_KEY_b:
            return WA_KEY_B;
        case XKB_KEY_N:
        case XKB_KEY_n:
            return WA_KEY_N;
        case XKB_KEY_M:
        case XKB_KEY_m:
            return WA_KEY_M;

        case XKB_KEY_0:
            return WA_KEY_0;
        case XKB_KEY_1:
            return WA_KEY_1;
        case XKB_KEY_2:
            return WA_KEY_2;
        case XKB_KEY_3:
            return WA_KEY_3;
        case XKB_KEY_4:
            return WA_KEY_4;
        case XKB_KEY_5:
            return WA_KEY_5;
        case XKB_KEY_6:
            return WA_KEY_6;
        case XKB_KEY_7:
            return WA_KEY_7;
        case XKB_KEY_8:
            return WA_KEY_8;
        case XKB_KEY_9:
            return WA_KEY_9;

        case XKB_KEY_Escape:
            return WA_KEY_ESC;
        case XKB_KEY_Tab:
            return WA_KEY_TAB;
        case XKB_KEY_Caps_Lock:
            return WA_KEY_CAPS;
        case XKB_KEY_Shift_L:
            return WA_KEY_LSHIFT;
        case XKB_KEY_Control_L:
            return WA_KEY_LCTRL;
        case XKB_KEY_Alt_L:
            return WA_KEY_LALT;
        case XKB_KEY_space:
            return WA_KEY_SPACE;
        case XKB_KEY_Alt_R:
            return WA_KEY_RALT;
        case XKB_KEY_Shift_R:
            return WA_KEY_RSHIFT;
        case XKB_KEY_Return:
            return WA_KEY_ENTER;
        case XKB_KEY_UP:
            return WA_KEY_UP;
        case XKB_KEY_Left:
            return WA_KEY_LEFT;
        case XKB_KEY_Right:
            return WA_KEY_RIGHT;
        case XKB_KEY_DOWN:
            return WA_KEY_DOWN;
        default:
            return WA_KEY_NONE;
    }
}

void 
wa_xkb_map(wa_window_t* window, u32 size, i32 fd)
{
    char* map_str = MAP_FAILED;

    map_str = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map_str == MAP_FAILED)
    {
        close(fd);
        return;
    }

    if (window->xkb_keymap)
        xkb_keymap_unref(window->xkb_keymap);
    if (window->xkb_state)
        xkb_state_unref(window->xkb_state);

    window->xkb_keymap = xkb_keymap_new_from_string(
            window->xkb_context,
            map_str,
            XKB_KEYMAP_FORMAT_TEXT_V1,
            0
    );

    munmap(map_str, size);
    close(fd);

    if (window->xkb_keymap == NULL)
    {
        fprintf(stderr, "WA ERROR: Failed to compile keymap.\n");
        return;
    }

    window->xkb_state = xkb_state_new(window->xkb_keymap);
    if (window->xkb_state == NULL)
    {
        fprintf(stderr, "WA ERROR: Failed to create XKB state.\n");
        xkb_keymap_unref(window->xkb_keymap);
        window->xkb_keymap = NULL;
        return;
    }
}

wa_key_t
wa_xkb_key(wa_window_t* window, u32 key, u32 state)
{
    xkb_keycode_t keycode = key + 8;
    xkb_keysym_t sym = xkb_state_key_get_one_sym(window->xkb_state, keycode);
    u8 pressed = state & WL_KEYBOARD_KEY_STATE_PRESSED;
    wa_key_t wa_key = wa_xkb_to_wa_key(sym);

    window->state.key_map[wa_key] = pressed;

    return wa_key;
}

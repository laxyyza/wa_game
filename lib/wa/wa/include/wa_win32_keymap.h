#ifndef _WA_WIN32_KEYMAP_H_
#define _WA_WIN32_KEYMAP_H_

#include "wa_keys.h"
#include <windows.h>

wa_key_t wa_win32_key_to_wa_key(WPARAM wparam);

#endif // _WA_WIN32_KEYMAP_H_

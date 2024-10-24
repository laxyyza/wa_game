#ifndef _WA_XKBCOMMON_H_
#define _WA_XKBCOMMON_H_

#include "wa.h"
#include "wa_keys.h"
#include <xkbcommon/xkbcommon.h>

wa_key_t wa_xkb_to_wa_key(xkb_keysym_t keysym);
void wa_xkb_map(wa_window_t* window, uint32_t size, int32_t fd);
wa_key_t wa_xkb_key(wa_window_t* window, uint32_t key, uint32_t state);

#endif // _WA_XKBCOMMON_H_

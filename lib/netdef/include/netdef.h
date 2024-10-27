#ifndef _NETDEF_H_
#define _NETDEF_H_

#include "coregame.h"
#include "ssp.h"

enum segtypes
{
	NET_TCP_CONNECT,
	NET_TCP_SESSION_ID,
	NET_DEBUG_MSG,

	NET_SEGTYPES_LEN
};

typedef struct 
{
	coregame_t* coregame;
	ssp_state_t ssp_state;
} netdef_t;

void netdef_init(netdef_t* netdef, coregame_t* coregame, 
				 const ssp_segmap_callback_t callbacks_override[NET_SEGTYPES_LEN]);

#endif // _NETDEF_H_


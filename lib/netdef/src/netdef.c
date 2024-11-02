#include "netdef.h"
#include <stdio.h>

void 
tcp_debug_msg(const ssp_segment_t* segment, UNUSED void* user_data, UNUSED void* source_data)
{
	const char* msg = (const char*)segment->data;

	printf("TCP Debug Message: '%s'\n", msg);
}

void 
netdef_init(netdef_t* netdef, coregame_t* coregame, 
			const ssp_segmap_callback_t callbacks_override[NET_SEGTYPES_LEN])
{
	netdef->coregame = coregame;
	ssp_state_init(&netdef->ssp_state);

	ssp_segmap(&netdef->ssp_state, NET_DEBUG_MSG, tcp_debug_msg);

	if (callbacks_override)
	{
		for (u32 i = 0; i < NET_SEGTYPES_LEN; i++)
		{
			const ssp_segmap_callback_t* callback = callbacks_override + i;
			if (*callback != NULL)
				ssp_segmap(&netdef->ssp_state, i, *callback);
		}
	}
}

void 
netdef_destroy(netdef_t* netdef)
{
	ssp_state_destroy(&netdef->ssp_state);
}

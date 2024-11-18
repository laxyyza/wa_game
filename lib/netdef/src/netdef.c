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
	netdef->ssp_state.segment_type_str = (void*)netdef_segtypes_str;

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

const char* 
netdef_segtypes_str(enum segtypes type)
{
	switch (type)
	{
		case NET_TCP_CONNECT:
			return "NET_TCP_CONNECT";
		case NET_TCP_SESSION_ID:
			return "NET_TCP_SESSION_ID";
		case NET_TCP_NEW_PLAYER:
			return "NET_TCP_NEW_PLAYER";
		case NET_TCP_DELETE_PLAYER:
			return "NET_TCP_DELETE_PLAYER";
		case NET_TCP_UDP_INFO:
			return "NET_TCP_UDP_INFO";
		case NET_TCP_SERVER_SHUTDOWN:
			return "NET_TCP_SERVER_SHUTDOWN";
		case NET_TCP_CG_MAP:
			return "NET_TCP_CG_MAP";
		case NET_TCP_WANT_SERVER_STATS:
			return "NET_TCP_WANT_SERVER_STATS";
		case NET_TCP_CHAT_MSG:
			return "NET_TCP_CHAT_MSG";
		case NET_TCP_GUN_SPEC:
			return "NET_TCP_GUN_SPEC";
		case NET_UDP_PLAYER_MOVE:
			return "NET_UDP_PLAYER_MOVE";
		case NET_UDP_PLAYER_CURSOR:
			return "NET_UDP_PLAYER_CURSOR";
		case NET_UDP_PLAYER_HEALTH:
			return "NET_UDP_PLAYER_HEALTH";
		case NET_UDP_PLAYER_STATS:
			return "NET_UDP_PLAYER_STATS";
		case NET_UDP_PLAYER_DIED:
			return "NET_UDP_PLAYER_DIED";
		case NET_UDP_PLAYER_PING:
			return "NET_UDP_PLAYER_PING";
		case NET_UDP_PLAYER_GUN_ID:
			return "NET_UDP_PLAYER_GUN_ID";
		case NET_UDP_PLAYER_INPUT:
			return "NET_UDP_PLAYER_INPUT";
		case NET_UDP_PING:
			return "NET_UDP_PING";
		case NET_UDP_PONG:
			return "NET_UDP_PONG";
		case NET_UDP_SERVER_STATS:
			return "NET_UDP_SERVER_STATS";
		case NET_UDP_DO_RECONNECT:
			return "NET_UDP_DO_RECONNECT";
		case NET_DEBUG_MSG:
			return "NET_DEBUG_MSG";
		default:
			return "Unknown";
	}
}

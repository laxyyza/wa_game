#ifndef _NETDEF_H_
#define _NETDEF_H_

#include "coregame.h"
#include "ssp.h"

enum segtypes
{
	NET_TCP_CONNECT,
	NET_TCP_SESSION_ID,
	NET_DEBUG_MSG,
	NET_TCP_NEW_PLAYER,
	NET_TCP_DELETE_PLAYER,

	NET_SEGTYPES_LEN
};

typedef struct 
{
	char username[PLAYER_NAME_MAX];
} net_tcp_connect_t;

typedef struct 
{
	u32 session_id; 
	u32 player_id;
} net_tcp_sessionid_t;

typedef struct 
{
	u32 player_id;
} net_tcp_delete_player_t;

typedef struct cg_player net_tcp_new_player_t;

typedef struct 
{
	coregame_t* coregame;
	ssp_state_t ssp_state;
} netdef_t;

void netdef_init(netdef_t* netdef, coregame_t* coregame, 
				 const ssp_segmap_callback_t callbacks_override[NET_SEGTYPES_LEN]);

#endif // _NETDEF_H_


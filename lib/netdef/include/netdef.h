#ifndef _NETDEF_H_
#define _NETDEF_H_

#include "coregame.h"
#include "ssp.h"
#include "ssp_tcp.h"

enum segtypes
{
	NET_TCP_CONNECT,
	NET_TCP_SESSION_ID,
	NET_TCP_NEW_PLAYER,
	NET_TCP_DELETE_PLAYER,
	NET_TCP_UDP_INFO,

	NET_UDP_PLAYER_MOVE,
	NET_UDP_PLAYER_SHOOT,

	NET_DEBUG_MSG,

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

typedef struct 
{
	u16 port;
} net_tcp_udp_info_t;

typedef struct 
{
	u32		player_id;
	vec2f_t pos;
	vec2f_t dir;
} net_udp_player_move_t;

typedef struct 
{
	u32 player_id;
	vec2f_t shoot_dir;
	vec2f_t shoot_pos;
} net_udp_player_shoot_t;

typedef struct cg_player net_tcp_new_player_t;

typedef struct 
{
	coregame_t* coregame;
	ssp_state_t ssp_state;
} netdef_t;

typedef struct 
{
	struct sockaddr_in addr;
	socklen_t addr_len;
} udp_addr_t;

void netdef_init(netdef_t* netdef, coregame_t* coregame, 
				 const ssp_segmap_callback_t callbacks_override[NET_SEGTYPES_LEN]);

#endif // _NETDEF_H_


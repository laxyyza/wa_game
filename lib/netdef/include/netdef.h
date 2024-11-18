#ifndef _NETDEF_H_
#define _NETDEF_H_

#include "coregame.h"
#include "ssp.h"
#include "ssp_tcp.h"

#define DEFAULT_PORT 49420
#define CHAT_MSG_MAX 128

enum segtypes
{
	NET_TCP_CONNECT,
	NET_TCP_SESSION_ID,
	NET_TCP_NEW_PLAYER,
	NET_TCP_DELETE_PLAYER,
	NET_TCP_UDP_INFO,
	NET_TCP_SERVER_SHUTDOWN,
	NET_TCP_CG_MAP,
	NET_TCP_WANT_SERVER_STATS,
	NET_TCP_CHAT_MSG,
	NET_TCP_GUN_SPEC,

	NET_UDP_PLAYER_MOVE,
	NET_UDP_PLAYER_CURSOR,
	NET_UDP_PLAYER_HEALTH,
	NET_UDP_PLAYER_STATS,
	NET_UDP_PLAYER_DIED,
	NET_UDP_PLAYER_PING,
	NET_UDP_PLAYER_GUN_ID,
	NET_UDP_PLAYER_INPUT,

	NET_UDP_PING,
	NET_UDP_PONG,

	NET_UDP_SERVER_STATS,
	NET_UDP_DO_RECONNECT,

	NET_DEBUG_MSG,

	NET_SEGTYPES_LEN
};

typedef cg_disk_map_t net_tcp_cg_map_t;
typedef cg_gun_spec_t net_tcp_gun_spec_t;

typedef struct 
{
	u8 flags;
	u32 player_id;
} net_udp_player_input_t;

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
	f64 tickrate;
	u8  ssp_flags;
} net_tcp_udp_info_t;

typedef struct 
{
	bool opt_in;
} net_tcp_want_server_stats_t;

typedef struct 
{
	u32 player_id;
	char msg[CHAT_MSG_MAX];
} net_tcp_chat_msg_t;

typedef struct 
{
	u32		player_id;
	vec2f_t pos;
	vec2f_t dir;
} net_udp_player_move_t;

typedef struct 
{
	vec2f_t cursor_pos;
	u32		player_id;
} net_udp_player_cursor_t;

typedef struct 
{
	u32 player_id;
	i32 health;
} net_udp_player_health_t;

typedef struct 
{
	u32 player_id;
	u16 kills;
	u16 deaths;
} net_udp_player_stats_t;

typedef struct 
{
	u32 target_player_id;
	u32 attacker_player_id;
} net_udp_player_died_t;

typedef struct 
{
	struct timespec start_time;
} net_udp_pingpong_t;

typedef struct 
{
	u32 player_id;
	f32 ms;
} net_udp_player_ping_t;

typedef struct 
{
	u32 gun_id;
	u32 player_id;
} net_udp_player_gun_id_t;

typedef struct 
{
	u32 id;
	u32 gun_id;
	vec2f_t pos;
	vec2f_t size;
	vec2f_t dir;
	vec2f_t cursor;
	f32 health;
	f32 max_health;
	char username[PLAYER_NAME_MAX];
	bool shoot;
} net_tcp_new_player_t;

typedef struct 
{
	coregame_t* coregame;
	ssp_state_t ssp_state;
} netdef_t;

typedef struct 
{
	struct sockaddr_in addr;
	socklen_t addr_len;
	char ipaddr[INET6_ADDRSTRLEN];
	u16 port;
} udp_addr_t;

typedef struct 
{
	u32 udp_pps_out;
	u32 udp_pps_out_highest;
	u32 udp_pps_out_bytes;
	u32 udp_pps_out_bytes_highest;

	u32 udp_pps_in;
	u32 udp_pps_in_highest;
	u32 udp_pps_in_bytes;
	u32 udp_pps_in_bytes_highest;

	i64 tick_time;
	i64 tick_time_avg;
	i64 tick_time_highest;
} server_stats_t, udp_server_stats_t;

void netdef_init(netdef_t* netdef, coregame_t* coregame, 
				 const ssp_segmap_callback_t callbacks_override[NET_SEGTYPES_LEN]);
void netdef_destroy(netdef_t* netdef);
const char* netdef_segtypes_str(enum segtypes type);

#endif // _NETDEF_H_


#ifndef _NETDEF_H_
#define _NETDEF_H_

#include "coregame.h"
#include "ssp.h"
#include "ssp_tcp.h"
#include "nano_timer.h"

#define DEFAULT_PORT 49420
#define CHAT_MSG_MAX 128

enum segtypes
{
	NET_TCP_CONNECT = 0,
	NET_TCP_SESSION_ID = 1,
	NET_TCP_NEW_PLAYER = 2,
	NET_TCP_DELETE_PLAYER = 3,
	NET_TCP_UDP_INFO = 4,
	NET_TCP_SERVER_SHUTDOWN = 5,
	NET_TCP_CG_MAP = 6,
	NET_TCP_WANT_SERVER_STATS = 7,
	NET_TCP_CHAT_MSG = 8,
	NET_TCP_GUN_SPEC = 9,

	NET_UDP_PLAYER_MOVE = 10,
	NET_UDP_PLAYER_CURSOR = 11,
	NET_UDP_PLAYER_HEALTH = 12,
	NET_UDP_PLAYER_STATS = 13,
	NET_UDP_PLAYER_DIED = 14,
	NET_UDP_PLAYER_PING = 15,
	NET_UDP_PLAYER_GUN_ID = 16,
	NET_UDP_PLAYER_INPUT = 17,
	NET_UDP_PLAYER_RELOAD = 18,
	NET_UDP_PLAYER_GUN_STATE = 19,

	NET_UDP_BULLET = 20,

	NET_UDP_PING = 21,
	NET_UDP_PONG = 22,

	NET_UDP_SERVER_STATS = 23,
	NET_UDP_DO_RECONNECT = 24,

	NET_DEBUG_MSG = 25,

	NET_SEGTYPES_LEN
};

typedef cg_disk_map_t net_tcp_cg_map_t;
typedef cg_gun_spec_t net_tcp_gun_spec_t;

typedef struct 
{
	u8 flags;
	f64 timestamp;
	u32 player_id;
} _SSP_PACKED net_udp_player_input_t;

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
	f64 time;
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
	u8		input;
	bool	absolute;
} _SSP_PACKED net_udp_player_move_t;

typedef struct 
{
	vec2f_t cursor_pos;
	u32		player_id;
} _SSP_PACKED net_udp_player_cursor_t;

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
} _SSP_PACKED net_udp_player_stats_t;

typedef struct 
{
	u32 target_player_id;
	u32 attacker_player_id;
} net_udp_player_died_t;

typedef struct 
{
	f64 t_client_ms;
	f64 t_server_ms;
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
	u32 player_id;
} net_udp_player_reload_t;

typedef struct
{
	u32 player_id;
	u8 gun_id;
	u16 ammo;	// ammo count 0 means reloading.
	f32 bullet_timer;
	f32 charge_timer;
	f32 reload_timer;
} _SSP_PACKED net_udp_player_gun_state_t;

typedef struct 
{
	u32		owner_id;
	vec2f_t pos;
	vec2f_t dir;
} net_udp_bullet_t;

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
	ssp_ctx_t ssp_ctx;
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
				 const ssp_segment_callback_t callbacks_override[NET_SEGTYPES_LEN]);
void netdef_destroy(netdef_t* netdef);
const char* netdef_segtypes_str(enum segtypes type);

#endif // _NETDEF_H_


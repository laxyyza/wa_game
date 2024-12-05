#ifndef _CLIENT_GAME_NET_EVENTS_H_
#define _CLIENT_GAME_NET_EVENTS_H_

#include "app.h"

void game_new_player(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _);
void game_delete_player(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _);
void game_player_move(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _);
void game_player_cursor(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _);
void game_player_health(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _);
void game_player_died(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _);
void game_player_shoot(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _);
void game_player_stats(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _);
void game_player_ping(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _);
void game_cg_map(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _);
void game_server_shutdown(UNUSED const ssp_segment_t* segment, waapp_t* app, UNUSED void* _);
void game_chat_msg(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _);
void game_gun_spec(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _);
void game_player_gun_id(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _);
void game_player_input(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _);
void game_player_reload(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _);
void game_player_gun_state(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _);
void game_username_change(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _);
void game_rewind_bullet(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _);
void udp_test(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _);
void game_bullet(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _);
void game_player_gun_state(const ssp_segment_t* segment, waapp_t* app, UNUSED void* _);

#endif // _CLIENT_GAME_NET_EVENTS_H_

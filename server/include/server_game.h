#ifndef _SERVER_GAME_H_
#define _SERVER_GAME_H_

#include "server.h"

void player_ping(const ssp_segment_t* segment, server_t* server, client_t* source_client);
void udp_ping(const ssp_segment_t* segment, server_t* server, client_t* source_client);
void player_shoot(const ssp_segment_t* segment, server_t* server, client_t* source_client);
void player_cursor(const ssp_segment_t* segment, server_t* server, client_t* source_client);
void player_dir(const ssp_segment_t* segment, UNUSED server_t* server, client_t* source_client);
void client_tcp_connect(const ssp_segment_t* segment, server_t* server, client_t* client);
void on_player_damaged(cg_player_t* target_player, cg_player_t* attacker_player, server_t* server);
void on_player_changed(cg_player_t* player, server_t* server);
void broadcast_delete_player(server_t* server, u32 id);
void want_server_stats(const ssp_segment_t* segment, UNUSED server_t* server, client_t* source_client);
void chat_msg(const ssp_segment_t* segment, server_t* server, client_t* source_client);

#endif // _SERVER_GAME_H_

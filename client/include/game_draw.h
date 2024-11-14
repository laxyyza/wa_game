#ifndef _CLIENT_GAME_DRAW_H_
#define _CLIENT_GAME_DRAW_H_

#include "game.h"
#include "cg_map.h"

void game_draw(client_game_t* game);
void game_render_map(waapp_t* app, cg_runtime_map_t* map, bool show_grid);

#endif // _CLIENT_GAME_DRAW_H_

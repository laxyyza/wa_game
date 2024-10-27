#ifndef _WAAPP_H_
#define _WAAPP_H_

#include <wa.h>
#include <stdbool.h>
#include "renderer.h"
#include "vec.h"
#include "util.h"
#include "player.h"
#include "client_net.h"

struct nk_wa;
struct nk_conext;

typedef struct waapp
{
    wa_window_t* window;
    vec4f_t bg_color;
    vec4f_t color;
    ren_t ren;

	texture_t* tank_bottom_tex;
	texture_t* tank_top_tex;

	rect_t world_border;
	coregame_t game;
	ght_t projectiles;
	ght_t players;
	player_t* player;
	vec2f_t prev_dir;
	vec2f_t prev_pos;
	bro_t* line_bro;

	client_net_t net;

    vec2f_t mouse;
    vec2f_t mouse_prev;
    vec3f_t cam;

    struct nk_wa* nk_wa;
    struct nk_context* nk_ctx;
} waapp_t;

i32 waapp_init(waapp_t* app, i32 argc, const char** argv);
void waapp_run(waapp_t* app);
void waapp_cleanup(waapp_t* app);

#endif // _WAAPP_H_

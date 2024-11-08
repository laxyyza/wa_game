#ifndef _WAAPP_H_
#define _WAAPP_H_

#include <wa.h>
#include <stdbool.h>
#include "renderer.h"
#include "vec.h"
#include "util.h"
#include "player.h"
#include "client_net.h"
#include "mmframes.h"
#include "state.h"
#include "nlog.h"
#include "game.h"

struct nk_wa;
struct nk_conext;

typedef struct waapp
{
    wa_window_t* window;
    vec4f_t bg_color;
    ren_t ren;

	texture_t* grass_tex;
	texture_t* block_tex;

    vec3f_t cam;
	client_game_t* game;

	client_net_t net;
	mmframes_t mmf;
	waapp_state_manager_t sm;

    vec2f_t mouse;
    vec2f_t mouse_prev;
	u32		fps;
	f64		frame_time;
	u32		frames;
	f32		max_fps;
	f64		fps_interval;
	bool	fps_limit;

	bool update_vync;
	bool tmp_vsync;
	bool on_ui;
	bool clamp_cam;

    struct nk_wa* nk_wa;
    struct nk_context* nk_ctx;

	struct timespec start_time;
	struct timespec end_time;
	struct timespec last_time;

	struct {
		wa_mouse_butt_t cam_move;
	} keybind;

	rect_t map_border;

	struct nk_font* font;
	struct nk_font* font_big;

	f32 min_zoom;
	f32 max_zoom;

	cg_map_t* map_from_server;
	const cg_map_t* current_map;

	bool disable_debug;
	i32 get_server_stats;
} waapp_t;

i32 waapp_init(waapp_t* app, i32 argc, char* const* argv);
void waapp_run(waapp_t* app);
void waapp_cleanup(waapp_t* app);
void waapp_set_max_fps(waapp_t* app, f64 max_fps);

ALWAYS_INLINE vec2f_t
screen_to_world(ren_t* ren, const vec2f_t* screen_pos)
{
	return vec2f(
		(screen_pos->x / ren->scale.x) - (ren->cam.x / ren->scale.x),
		(screen_pos->y / ren->scale.y) - (ren->cam.y / ren->scale.y)
	);
}

#endif // _WAAPP_H_

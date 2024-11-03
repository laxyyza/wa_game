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
	texture_t* grass_tex;
	texture_t* block_tex;

	coregame_t game;
	ght_t players;
	player_t* player;
	vec2f_t prev_dir;
	vec2f_t prev_pos;
	bro_t* line_bro;

	client_net_t net;
	mmframes_t mmf;
	waapp_state_manager_t sm;

    vec2f_t mouse;
    vec2f_t mouse_prev;
    vec3f_t cam;
	bool	lock_cam;
	bool	trigger_shooting;
	u32		fps;
	f64		frame_time;
	u32		frames;
	f32		max_fps;
	f64		fps_interval;
	bool	fps_limit;

	bool update_vync;
	bool tmp_vsync;
	bool on_ui;

    struct nk_wa* nk_wa;
    struct nk_context* nk_ctx;

	struct timespec start_time;
	struct timespec end_time;
	struct timespec last_time;

	struct {
		wa_mouse_butt_t cam_move;
	} keybind;

	f32 min_zoom;
	f32 max_zoom;

	rect_t map_border;

	struct nk_font* font;
	struct nk_font* font_big;

	array_t player_deaths;

	f64 death_kill_time;

	cg_map_t* map_from_server;
} waapp_t;

i32 waapp_init(waapp_t* app, i32 argc, const char** argv);
void waapp_run(waapp_t* app);
void waapp_cleanup(waapp_t* app);
void waapp_set_max_fps(waapp_t* app, f64 max_fps);
void waapp_lock_cam(waapp_t* app);
void waapp_move_cam(waapp_t* app);

ALWAYS_INLINE vec2f_t
screen_to_world(waapp_t* app, const vec2f_t* screen_pos)
{
	return vec2f(
		(screen_pos->x / app->ren.scale.x) - (app->cam.x / app->ren.scale.x),
		(screen_pos->y / app->ren.scale.y) - (app->cam.y / app->ren.scale.y)
	);
}

void game_init(waapp_t* app, void* data);
void game_update(waapp_t* app, void* data);
i32  game_event(waapp_t* app, const wa_event_t* ev);
void game_cleanup(waapp_t* app, void* data);
void game_handle_mouse_wheel(waapp_t* app, const wa_event_wheel_t* ev);

#endif // _WAAPP_H_

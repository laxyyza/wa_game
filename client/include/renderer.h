#ifndef _RENDERER_H_
#define _RENDERER_H_

#include "mat.h"
#include "idxbuf.h"
#include "shader.h"
#include "vertarray.h"
#include <ght.h>
#include "rect.h"

#define TRIA_VERT 3
#define NO_TEXTURE -1

typedef struct renderer ren_t;
typedef struct batch_render_obj bro_t;
typedef void (*ren_draw_rect_t)(ren_t* ren, bro_t* bro, const rect_t* rect);
typedef void (*ren_draw_misc_t)(ren_t* ren, bro_t* bro, const void* data);
typedef void (*ren_draw_line_t)(ren_t* ren, 
								bro_t* bro, 
								const vec2f_t* a, 
								const vec2f_t* b, 
								u32 color32);

enum draw_mode
{
    DRAW_TRIANGLES,
    DRAW_LINES,
};

typedef struct default_vertex
{
    vec4f_t pos;
    vec4f_t color;
    vec2f_t tex_cords;
    f32     texture_id;
} default_vertex_t;

typedef struct line_vertex
{
    vec4f_t pos;
    vec4f_t color;
} line_vertex_t;

typedef struct 
{
	vec2f_t pos;
	vec4f_t color;
} projectile_vertex_t;

typedef struct 
{
	vec2f_t pos;
	vec2f_t pos_a;
	vec2f_t pos_b;
} laser_vertex_t;

typedef struct batch_render_obj
{
    vertarray_t vao;
    vertbuf_t vbo;
    vertlayout_t layout;
    idxbuf_t ibo;
    shader_t shader;
    bool textures_changed;
	bool shared_shader;
	array_t current_textures;

    u32 draw_mode;

	ren_draw_rect_t draw_rect;
	ren_draw_misc_t draw_misc;
	ren_draw_line_t draw_line;
} bro_t;

enum vertlayout 
{
	VERTLAYOUT_END = -1,
	VERTLAYOUT_F32,
	VERTLAYOUT_I32,
	VERTLAYOUT_U32,
};

typedef struct 
{
	enum draw_mode draw_mode;
	u32 max_vb_count;

	const char* vert_path;
	const char* frag_path;
	const shader_t* shader;

	u32 vertex_size;

	const i32* vertlayout;
	
	ren_draw_rect_t draw_rect;
	ren_draw_misc_t draw_misc;
	ren_draw_line_t draw_line;
} bro_param_t;

typedef struct renderer
{
    mat4_t proj;
    mat4_t view;
    mat4_t model;
    mat4_t mvp;
	mat4_t scale_mat;
    vec2f_t scale;

    bro_t* default_bro; // Default Batch Render Object
	bro_t* line_bro;
    bro_t* current_bro;

	array_t mvp_shaders;

    u32 draw_calls;

    u32     max_texture_units;

	vec2f_t viewport;
	vec2f_t cam;
} ren_t;

void ren_init(ren_t* ren);
void ren_del(ren_t* ren);
void ren_viewport(ren_t* ren, i32 w, i32 h);
void bro_bind_submit(bro_t* bro);
bro_t* ren_new_bro(ren_t* ren, const bro_param_t* param);
void ren_bind_bro(ren_t* ren, bro_t* bro);
void ren_delete_bro(bro_t* bro);

void ren_clear(ren_t* ren, const vec4f_t* color);
void bro_reset(bro_t* bro);
void ren_draw_rect(ren_t* ren, const rect_t* rect);
void ren_draw_line(ren_t* ren, const vec2f_t* a, const vec2f_t* b, u32 color);
void ren_draw_batch(ren_t* ren);


void ren_set_view(ren_t* ren, const vec3f_t* view);
void ren_set_scale(ren_t* ren, const vec2f_t* scale);

void bro_set_mvp(ren_t* ren, bro_t* bro);
void ren_default_draw_rect(ren_t* ren, bro_t* bro, const rect_t* rect);
void ren_default_draw_line(ren_t* ren, bro_t* bro, const vec2f_t* a, const vec2f_t* b, u32 color32);
void main_menu_draw_rect(ren_t* ren, bro_t* bro, const rect_t* rect);
void ren_default_draw_rect_lines(ren_t* ren, bro_t* bro, const rect_t* rect);
void bro_draw_batch(ren_t* ren, bro_t* bro);
void ren_laser_draw_misc(ren_t* ren, bro_t* bro, const void* draw_data);

#endif // _RENDERER_H_

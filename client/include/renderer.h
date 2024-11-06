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

enum draw_mode
{
    DRAW_TRIANGLES,
    DRAW_LINES,
};

typedef struct vertex
{
    vec4f_t pos;
    vec4f_t color;
    vec2f_t tex_cords;
    f32     texture_id;
} vertex_t;

typedef struct 
{
	vec2f_t pos;
	vec4f_t color;
} projectile_vertex_t;

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
} bro_t;

enum vertlayout 
{
	VERTLAYOUT_END = 0,
	VERTLAYOUT_F32,
	VERTLAYOUT_I32,
	VERTLAYOUT_U32,
};

typedef struct renderer
{
    mat4_t proj;
    mat4_t view;
    mat4_t model;
    mat4_t mvp;
	mat4_t scale_mat;
    vec2f_t scale;

    bro_t* default_bro; // Default Batch Render Object
    bro_t* current_bro;

    // vertex_t* vertices;
    // u32 vertices_used;
    // vertbuf_t vertbuf;
    // idxbuf_t idxbuf;
    // vertarray_t vao;
    // vertlayout_t layout;
    // shader_t shader;
    // array_t ibo;
    u32 draw_calls;

    // ght_t textures;
    // bool textures_changed;

    u32     max_texture_units;

	vec2f_t viewport;
	vec2f_t cam;
} ren_t;

void ren_init(ren_t* ren);
void ren_del(ren_t* ren);
void ren_viewport(ren_t* ren, i32 w, i32 h);
void bro_bind_submit(bro_t* bro);
bro_t* ren_new_bro(enum draw_mode mode, u32 max_vb_count, const char* vert_shader, const char* frad_shader, const shader_t* shared_shader);
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

#endif // _RENDERER_H_

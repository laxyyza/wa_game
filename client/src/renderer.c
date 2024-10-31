#define _USE_MATH_DEFINES
#include "renderer.h"
#include "array.h"
#include "ght.h"
#include "idxbuf.h"
#include "opengl.h"
#include "texture.h"
#include "vec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define INITIAL_IBO 16
#define MAX_VERTICES 4096

static void 
enable_blending(void)
{
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

static void
set_uniform_textures(bro_t* bro)
{
    if (bro->current_textures.count == 0)
	{
        return;
	}

	u32 tex_count = bro->current_textures.count;
    i32* indexes = malloc(tex_count * sizeof(u32));
	i32* texture_ids = (i32*)bro->current_textures.buf;

    for (u32 i = 0; i < tex_count; i++)
    {
        glBindTextureUnit(i, texture_ids[i]);
        indexes[i] = i;
    }

    shader_uniform1iv(&bro->shader, "u_textures", (i32*)indexes, tex_count);

    free(indexes);
	array_clear(&bro->current_textures, false);
}

static void
ren_def_bro(ren_t* ren)
{
    ren->default_bro = ren_new_bro(DRAW_TRIANGLES,
                                    MAX_VERTICES,
                                    "client/src/shaders/vert.glsl",
                                    "client/src/shaders/frag.glsl", NULL);
}

void
bro_bind_submit(bro_t* bro)
{
    vertarray_bind(&bro->vao);

    idxbuf_bind(&bro->ibo);
    idxbuf_submit(&bro->ibo);

    vertbuf_bind(&bro->vbo);
    vertbuf_submit(&bro->vbo);

    shader_bind(&bro->shader);

    set_uniform_textures(bro);
}

bro_t*
ren_new_bro(enum draw_mode draw_mode, u32 max_vb_count, const char* vert_shader, const char* frag_shader, const shader_t* shared_shader)
{
    vertarray_t* vao;
    vertbuf_t* vbo;
    vertlayout_t* layout;
    idxbuf_t* ibo;
    shader_t* shader;

    bro_t* bro = calloc(1, sizeof(bro_t));
    vao = &bro->vao;
    vbo = &bro->vbo;
    layout = &bro->layout;
    ibo = &bro->ibo;
    shader = &bro->shader;
    if (draw_mode == DRAW_TRIANGLES)
        bro->draw_mode = GL_TRIANGLES;
    else if (draw_mode == DRAW_LINES)
        bro->draw_mode = GL_LINES;

    vertarray_init(vao);
    vertbuf_init(vbo, NULL, max_vb_count);

    vertlayout_init(layout);
    vertlayout_add_f32(layout, 4);    // 4 floats: position
    vertlayout_add_f32(layout, 4);    // 4 floats: color
    vertlayout_add_f32(layout, 2);    // 2 floats: Texture coords
    vertlayout_add_f32(layout, 1);    // 1 float: Texture ID
    vertarray_add(vao, vbo, layout);

    idxbuf_init(ibo, IDXBUF_UINT16, NULL, INITIAL_IBO);

	if (shared_shader)
	{
		memcpy(shader, shared_shader, sizeof(shader_t));
		bro->shared_shader = true;
	}
	else
	{
		shader_init(shader, vert_shader, frag_shader);
		bro->shared_shader = false;
	}

	array_init(&bro->current_textures, sizeof(u32), 10);

    return bro;
}

void 
ren_delete_bro(bro_t* bro)
{
	if (bro == NULL)
		return;

	if (bro->shared_shader == false)
		shader_del(&bro->shader);
    idxbuf_del(&bro->ibo);
    vertlayout_del(&bro->layout);
    vertbuf_del(&bro->vbo);
    vertarray_del(&bro->vao);
	array_del(&bro->current_textures);
}

void 
ren_init(ren_t* ren)
{
    enable_blending();
    ren_def_bro(ren);
    ren_bind_bro(ren, ren->default_bro);
	mat4_identity(&ren->scale_mat);

    ren->scale = vec2f(1, 1);
    ren_set_scale(ren, &ren->scale);

    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, (i32*)&ren->max_texture_units);
}

static void
ren_set_mvp(ren_t* ren)
{
    mat4_mul(&ren->mvp, &ren->view, &ren->proj);
    shader_bind(&ren->current_bro->shader);
    shader_uniform_mat4f(&ren->current_bro->shader, "mvp", &ren->mvp);
}

void 
ren_set_scale(ren_t* ren, const vec2f_t* vec)
{
	if (&ren->scale != vec)
        memcpy(&ren->scale, vec, sizeof(vec2f_t));

	vec3f_t view = {ren->cam.x, ren->cam.y, 1.0};
	ren_set_view(ren, &view);
}

void 
ren_set_view(ren_t* ren, const vec3f_t* view)
{
    mat4_identity(&ren->view);
	mat4_scale(&ren->view, ren->scale.x, ren->scale.y, 1.0);
	mat4_translate(&ren->view, view);
    ren_set_mvp(ren);
	ren->cam.x = view->x;
	ren->cam.y = view->y;
}

void 
ren_viewport(ren_t* ren, i32 w, i32 h)
{
    glViewport(0, 0, w, h);
    ren->proj = mat_ortho(0, w, h, 0, -1.0, 1.0);
    ren_set_mvp(ren);
	ren->viewport.x = w;
	ren->viewport.y = h;
}

void 
ren_bind_bro(ren_t* ren, bro_t* bro)
{
    ren->current_bro = bro;
}

static void
ren_add_rect_indices(bro_t* bro, u32 v)
{
    array_t* ib_array = &bro->ibo.array;

    if (bro->draw_mode == GL_LINES)
    {
        // Top Left -> Right
        array_add_i16(ib_array, 0 + v);
        array_add_i16(ib_array, 1 + v);

        // Top Right -> Bot Right
        array_add_i16(ib_array, 1 + v);
        array_add_i16(ib_array, 2 + v);

        // Bot Right -> Bot Left
        array_add_i16(ib_array, 2 + v);
        array_add_i16(ib_array, 3 + v);

        // Bot Left -> Top Left
        array_add_i16(ib_array, 3 + v);
        array_add_i16(ib_array, 0 + v);
    }
    else if (bro->draw_mode == GL_TRIANGLES)
    {
        array_add_i16(ib_array, 0 + v);
        array_add_i16(ib_array, 1 + v);
        array_add_i16(ib_array, 2 + v);
        array_add_i16(ib_array, 2 + v);
        array_add_i16(ib_array, 3 + v);
        array_add_i16(ib_array, 0 + v);
    }
    else
    {
        assert(false);
    }
}

static f32 
bro_texture_idx(bro_t* bro, texture_t* texture)
{
    if (texture == NULL)
        return NO_TEXTURE;

    f32 ret;
	u32* texture_ids = (u32*)bro->current_textures.buf;

    for (u32 i = 0; i < bro->current_textures.count; i++)
	{
        if (texture_ids[i] == texture->id)
            return (f32)i;
	}
	ret = (f32)bro->current_textures.count;
	array_add_i32(&bro->current_textures, texture->id);
    return ret;
}

static void 
ren_draw_rect_norm(ren_t* ren, const rect_t* rect)
{
    bro_t* bro = ren->current_bro;

    if (bro->vbo.count + RECT_VERT > bro->vbo.max_count)
        ren_draw_batch(ren);

    const vec2f_t* pos = &rect->pos;
    const vec2f_t* size = &rect->size;
    const vec4f_t* color = &rect->color;
    const u32 v = bro->vbo.count;
    vertex_t* vertices = bro->vbo.buf + v;

    i32 texture_idx = bro_texture_idx(bro, rect->texture);

    if (bro->draw_mode == GL_LINES)
        texture_idx = NO_TEXTURE;

    vertices->pos = vec4f(rect->pos.x, rect->pos.y, 0, 1);
    vertices->color = *color;
    vertices->tex_cords = vec2f(0.0, 0.0);
    vertices->texture_id = texture_idx;
    vertices++;
    
    vertices->pos = vec4f(pos->x + size->x, pos->y, 0, 1);
    vertices->color = *color;
    vertices->tex_cords = vec2f(1.0, 0.0);
    vertices->texture_id = texture_idx;
    vertices++;

    vertices->pos = vec4f(pos->x + size->x, pos->y + size->y, 0, 1);
    vertices->color = *color;
    vertices->tex_cords = vec2f(1.0, 1.0);
    vertices->texture_id = texture_idx;
    vertices++;

    vertices->pos = vec4f(pos->x, pos->y + size->y, 0, 1);
    vertices->color = *color;
    vertices->tex_cords = vec2f(0.0, 1.0);
    vertices->texture_id = texture_idx;

    ren_add_rect_indices(bro, v);

    bro->vbo.count += RECT_VERT;
}

static bool 
rect_in_frustum(ren_t* ren, const rect_t* rect)
{
	rect_t frustum = {
		.pos.x = -ren->cam.x / ren->scale.x,
		.pos.y = -ren->cam.y / ren->scale.y,
		.size.x = ren->viewport.x / ren->scale.x,
		.size.y = ren->viewport.y / ren->scale.y,
	};

	if (rect->pos.x + rect->size.x < frustum.pos.x ||
		rect->pos.x > frustum.pos.x + frustum.size.x ||
		rect->pos.y + rect->size.y < frustum.pos.y ||
		rect->pos.y > frustum.pos.y + frustum.size.y)
	{
		return false;
	}

	return true;
}

void 
ren_draw_rect(ren_t* ren, const rect_t* rect)
{
	if (rect_in_frustum(ren, rect) == false)
		return;

	if (rect->rotation == 0)
	{
		ren_draw_rect_norm(ren, rect);
		return;
	}

    bro_t* bro = ren->current_bro;

    if (bro->vbo.count + RECT_VERT > bro->vbo.max_count || 
        bro->current_textures.count >= ren->max_texture_units)
        ren_draw_batch(ren);

    const vec2f_t* size = (vec2f_t*)&rect->size;
    const vec2f_t pos = rect_origin(rect);
    const vec4f_t* color = &rect->color;
    const u32 v = bro->vbo.count;
    vertex_t* vertices = bro->vbo.buf + v;

    f32 texture_idx = bro_texture_idx(bro, rect->texture);

    // f32 texture_id = (rect->texture) ? rect->texture->id: NO_TEXTURE;
    if (bro->draw_mode == GL_LINES)
        texture_idx = NO_TEXTURE;

    mat4_t transform;
    mat4_t rotation;
    mat4_t scale;

    mat4_identity(&rotation);

    mat4_rotate(&rotation, &rotation, rect->rotation);
    mat4_scale_vec2f(&scale, size);

    mat4_mul(&transform, &rotation, &scale);
    transform.m[0][3] = pos.x;
    transform.m[1][3] = pos.y;

    static const vec4f_t verts[4] = {
        {-0.5, -0.5, 0.0, 1.0},
        { 0.5, -0.5, 0.0, 1.0},
        { 0.5,  0.5, 0.0, 1.0},
        {-0.5,  0.5, 0.0, 1.0},
    };

    mat4_mul_vec4f(&vertices->pos, &transform, &verts[0]);
    vertices->color = *color;
    vertices->tex_cords = vec2f(0.0, 0.0);
    vertices->texture_id = texture_idx;
    vertices++;
    
    mat4_mul_vec4f(&vertices->pos, &transform, &verts[1]);
    vertices->color = *color;
    vertices->tex_cords = vec2f(1.0, 0.0);
    vertices->texture_id = texture_idx;
    vertices++;

    mat4_mul_vec4f(&vertices->pos, &transform, &verts[2]);
    vertices->color = *color;
    vertices->tex_cords = vec2f(1.0, 1.0);
    vertices->texture_id = texture_idx;
    vertices++;

    mat4_mul_vec4f(&vertices->pos, &transform, &verts[3]);
    vertices->color = *color;
    vertices->tex_cords = vec2f(0.0, 1.0);
    vertices->texture_id = texture_idx;

    ren_add_rect_indices(bro, v);

    bro->vbo.count += RECT_VERT;
}

void 
ren_draw_line(ren_t* ren, const vec2f_t* a, const vec2f_t* b, u32 color32)
{
    bro_t* bro = ren->current_bro;

    if (bro->vbo.count + 2 > bro->vbo.max_count)
        ren_draw_batch(ren);

    const vec4f_t color = rgba(color32);
    const u32 v = bro->vbo.count;
    vertex_t* vertices = bro->vbo.buf + v;

    f32 texture_id = NO_TEXTURE;

    vertices->pos = vec4f(a->x, a->y, 0, 1);
    vertices->color = color;
    vertices->tex_cords = vec2f(0.0, 0.0);
    vertices->texture_id = texture_id;
    vertices++;
    
    vertices->pos = vec4f(b->x, b->y, 0, 1);
    vertices->color = color;
    vertices->tex_cords = vec2f(1.0, 0.0);
    vertices->texture_id = texture_id;

	array_add_i16(&bro->ibo.array, 0 + v);
	array_add_i16(&bro->ibo.array, 1 + v);

    bro->vbo.count += 2;
}

void 
ren_clear(ren_t* ren, const vec4f_t* c)
{
    glClearColor(c->x, c->y, c->z, c->w);
    glClear(GL_COLOR_BUFFER_BIT);
    ren->draw_calls = 0;
}

void
bro_reset(bro_t* bro)
{
    array_clear(&bro->ibo.array, false);

    bro->vbo.count = 0;
}

void 
ren_draw_batch(ren_t* ren)
{
    bro_t* bro = ren->current_bro;

    bro_bind_submit(bro);
    const idxbuf_t* ib = &bro->ibo;

    glDrawElements(bro->draw_mode, ib->array.count, ib->type, NULL);

    bro_reset(bro);
    ren->draw_calls++;
}

void 
ren_del(ren_t* ren)
{
    // vertbuf_unmap(&ren->vertbuf);
    ren_delete_bro(ren->default_bro);
}

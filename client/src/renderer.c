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

void 
ren_default_draw_line(ren_t* ren, bro_t* bro, const vec2f_t* a, const vec2f_t* b, u32 color32)
{
	bool do_batch = (bro->vbo.count + 2) >= bro->vbo.max_count;
	if (do_batch)
		bro_draw_batch(ren, bro);

    const vec4f_t color = rgba(color32);
    const u32 v = bro->vbo.count;
    line_vertex_t* vertices = ((line_vertex_t*)bro->vbo.buf) + v;

    vertices->pos = vec4f(a->x, a->y, 0, 1);
    vertices->color = color;
    vertices++;
    
    vertices->pos = vec4f(b->x, b->y, 0, 1);
    vertices->color = color;

	array_add_i16(&bro->ibo.array, 0 + v);
	array_add_i16(&bro->ibo.array, 1 + v);

    bro->vbo.count += 2;
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
ren_init_default_bro(ren_t* ren)
{
	const i32 layout[] = {
		VERTLAYOUT_F32, 4, // position
		VERTLAYOUT_F32, 4, // color
		VERTLAYOUT_F32, 2, // Texture Coords 
		VERTLAYOUT_F32, 1, // Texture ID
		VERTLAYOUT_END
	};
	const bro_param_t param = {
		.draw_mode = DRAW_TRIANGLES,
		.max_vb_count = MAX_VERTICES,
		.vert_path = "client/src/shaders/default_vert.glsl",
		.frag_path = "client/src/shaders/default_frag.glsl",
		.shader = NULL,
		.vertlayout = layout,
		.vertex_size = sizeof(default_vertex_t),
		.draw_rect = ren_default_draw_rect,
		.draw_line = NULL
	};

    ren->default_bro = ren_new_bro(ren, &param);
}

static void 
ren_init_line_bro(ren_t* ren)
{
	const i32 layout[] = {
		VERTLAYOUT_F32, 4, // position
		VERTLAYOUT_F32, 4, // color
		VERTLAYOUT_END
	};
	const bro_param_t param = {
		.draw_mode = DRAW_LINES,
		.max_vb_count = 1024,
		.vert_path = "client/src/shaders/line_vert.glsl",
		.frag_path = "client/src/shaders/line_frag.glsl",
		.shader = NULL,
		.vertlayout = layout,
		.vertex_size = sizeof(line_vertex_t),
		.draw_rect = ren_default_draw_rect_lines,
		.draw_line = ren_default_draw_line,
	};
	ren->line_bro = ren_new_bro(ren, &param);
}

static void
ren_def_bro(ren_t* ren)
{
	ren_init_default_bro(ren);
	ren_init_line_bro(ren);
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

static void
ren_parse_vertlayout(vertlayout_t* vertex_layout, const i32* layout_array)
{
	while (*layout_array != VERTLAYOUT_END)
	{
		enum vertlayout layout = *layout_array;
		i32 count = *(++layout_array);
		if (layout == VERTLAYOUT_F32)
			vertlayout_add_f32(vertex_layout, count);
		else if (layout == VERTLAYOUT_I32)
			vertlayout_add_i32(vertex_layout, count);
		else if (layout  == VERTLAYOUT_U32)
			vertlayout_add_u32(vertex_layout, count);
		else
			assert(false);
		layout_array++;
	}
}

bro_t*
ren_new_bro(ren_t* ren, const bro_param_t* param)
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
    if (param->draw_mode == DRAW_TRIANGLES)
        bro->draw_mode = GL_TRIANGLES;
    else if (param->draw_mode == DRAW_LINES)
        bro->draw_mode = GL_LINES;

    vertarray_init(vao);
    vertbuf_init(vbo, NULL, param->max_vb_count, param->vertex_size);

    vertlayout_init(layout);
	ren_parse_vertlayout(layout, param->vertlayout);
    // vertlayout_add_f32(layout, 4);    // 4 floats: position
    // vertlayout_add_f32(layout, 4);    // 4 floats: color
    // vertlayout_add_f32(layout, 2);    // 2 floats: Texture coords
    // vertlayout_add_f32(layout, 1);    // 1 float: Texture ID
    vertarray_add(vao, vbo, layout);

    idxbuf_init(ibo, IDXBUF_UINT16, NULL, INITIAL_IBO);

	if (param->shader)
	{
		memcpy(shader, param->shader, sizeof(shader_t));
		bro->shared_shader = true;
	}
	else
	{
		shader_init(shader, param->vert_path, param->frag_path);
		bro->shared_shader = false;

		shader_bind(shader);
		if (shader_uniform_location(shader, "mvp") != -1)
		{
			array_add_voidp(&ren->mvp_shaders, shader);
		}
		shader_unbind();
	}

	array_init(&bro->current_textures, sizeof(u32), 10);

	bro->draw_rect = param->draw_rect;
	bro->draw_misc = param->draw_misc;
	bro->draw_line = param->draw_line;

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
	array_init(&ren->mvp_shaders, sizeof(shader_t**), 4);
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

	/**
	 *	TODO: Somehow need to remove shaders from ren->mvp_shaders when they get freed.
	 *	This assumes mvp_shaders are valid.
	 */
	shader_t** shaders = (shader_t**)ren->mvp_shaders.buf;
	shader_t* shader;

	for (u32 i = 0; i < ren->mvp_shaders.count; i++)
	{
		shader = shaders[i];
		shader_bind(shader);
		shader_uniform_mat4f(shader, "mvp", &ren->mvp);
	}
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
ren_default_draw_rect_norm(ren_t* ren, bro_t* bro, const rect_t* rect)
{
    if (bro->vbo.count + RECT_VERT > bro->vbo.max_count)
        bro_draw_batch(ren, bro);

    const vec2f_t* pos = &rect->pos;
    const vec2f_t* size = &rect->size;
    const vec4f_t* color = &rect->color;
    const u32 v = bro->vbo.count;
    default_vertex_t* vertices = (default_vertex_t*)bro->vbo.buf + v;

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
	const rect_t frustum = {
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

static bool 
point_in_frustum(ren_t* ren, const vec2f_t* point)
{
	const rect_t frustum = {
		.pos.x = -ren->cam.x / ren->scale.x,
		.pos.y = -ren->cam.y / ren->scale.y,
		.size.x = ren->viewport.x / ren->scale.x,
		.size.y = ren->viewport.y / ren->scale.y,
	};

	if (point->x < frustum.pos.x || 
		point->y < frustum.pos.y || 
		point->x > frustum.pos.x + frustum.size.x ||
		point->y > frustum.pos.y + frustum.size.y)
	{
		return false;
	}
	return true;
}

void 
ren_default_draw_rect_lines(ren_t* ren, bro_t* bro, const rect_t* rect)
{
    if (bro->vbo.count + RECT_VERT > bro->vbo.max_count || 
        bro->current_textures.count >= ren->max_texture_units)
        bro_draw_batch(ren, bro);

    const vec2f_t* size = (vec2f_t*)&rect->size;
    const vec2f_t pos = rect_origin(rect);
    const vec4f_t* color = &rect->color;
    const u32 v = bro->vbo.count;
    line_vertex_t* vertices = (line_vertex_t*)bro->vbo.buf + v;

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
    vertices++;
    
    mat4_mul_vec4f(&vertices->pos, &transform, &verts[1]);
    vertices->color = *color;
    vertices++;

    mat4_mul_vec4f(&vertices->pos, &transform, &verts[2]);
    vertices->color = *color;
    vertices++;

    mat4_mul_vec4f(&vertices->pos, &transform, &verts[3]);
    vertices->color = *color;

    ren_add_rect_indices(bro, v);

    bro->vbo.count += RECT_VERT;
}

void 
ren_default_draw_rect(ren_t* ren, bro_t* bro, const rect_t* rect)
{
	if (rect_in_frustum(ren, rect) == false)
		return;

	if (rect->rotation == 0)
	{
		ren_default_draw_rect_norm(ren, bro, rect);
		return;
	}

    if (bro->vbo.count + RECT_VERT > bro->vbo.max_count || 
        bro->current_textures.count >= ren->max_texture_units)
        bro_draw_batch(ren, bro);

    const vec2f_t* size = (vec2f_t*)&rect->size;
    const vec2f_t pos = rect_origin(rect);
    const vec4f_t* color = &rect->color;
    const u32 v = bro->vbo.count;
    default_vertex_t* vertices = (default_vertex_t*)bro->vbo.buf + v;

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
ren_draw_rect(ren_t* ren, const rect_t* rect)
{
	bro_t* bro = ren->current_bro;

	if (bro->draw_rect)
		bro->draw_rect(ren, bro, rect);
	else
		printf("draw_rect() bro doesn't support draw_rect!\n");
}

void 
ren_draw_line(ren_t* ren, const vec2f_t* a, const vec2f_t* b, u32 color32)
{
    bro_t* bro = ren->line_bro;

	bro->draw_line(ren, bro, a, b, color32);
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
bro_draw_batch(ren_t* ren, bro_t* bro)
{
	if (bro->vbo.count == 0)
		return;

    bro_bind_submit(bro);
    const idxbuf_t* ib = &bro->ibo;

    glDrawElements(bro->draw_mode, ib->array.count, ib->type, NULL);

    bro_reset(bro);
    ren->draw_calls++;
}

void 
ren_draw_batch(ren_t* ren)
{
    bro_t* bro = ren->current_bro;
	bro_draw_batch(ren, bro);
}

void 
ren_del(ren_t* ren)
{
    // vertbuf_unmap(&ren->vertbuf);
	array_del(&ren->mvp_shaders);
    ren_delete_bro(ren->default_bro);
    ren_delete_bro(ren->line_bro);
}

void
main_menu_draw_rect(ren_t* ren, bro_t* bro, const rect_t* rect)
{
    if (bro->vbo.count + RECT_VERT > bro->vbo.max_count)
        ren_draw_batch(ren);

    const vec2f_t* pos = &rect->pos;
    const vec2f_t* size = &rect->size;
    const u32 v = bro->vbo.count;
    vec4f_t* vertices = (vec4f_t*)bro->vbo.buf + v;

    vertices[0] = vec4f(rect->pos.x, rect->pos.y, 0, 1);
    vertices[1] = vec4f(pos->x + size->x, pos->y, 0, 1);
    vertices[2] = vec4f(pos->x + size->x, pos->y + size->y, 0, 1);
    vertices[3] = vec4f(pos->x, pos->y + size->y, 0, 1);

    ren_add_rect_indices(bro, v);

    bro->vbo.count += RECT_VERT;
}

void 
ren_laser_draw_misc(ren_t* ren, bro_t* bro, const void* draw_data)
{
	const laser_draw_data_t* d_data = draw_data;
	const laser_vertex_t* data = &d_data->v;
	const laser_bullet_t* bullet_data = d_data->laser_data;

	bool in_frustum = point_in_frustum(ren, &data->pos_a);
	if (in_frustum == false)
		return;

    if (bro->vbo.count + RECT_VERT > bro->vbo.max_count)
        bro_draw_batch(ren, bro);

    const u32 v = bro->vbo.count;
	const f32 line_thicc = bullet_data->thickness;
	const vec2f_t* a = &data->pos_a;
	const vec2f_t* b = &data->pos_b;
    laser_vertex_t* vertices = (laser_vertex_t*)bro->vbo.buf + v;
	vec2f_t dir = {
		.x = b->x - a->x,
		.y = b->y - a->y
	};
	vec2f_norm(&dir);
	vec2f_t perpendicular = vec2f(-dir.y, dir.x);

	vec2f_t offset = {
		.x = (line_thicc / 2.0) * perpendicular.x,
		.y = (line_thicc / 2.0) * perpendicular.y
	};

	vertices->pos = vec2f(
		a->x - offset.x,
		a->y - offset.y
	);
	vertices->pos_a = data->pos_a;
	vertices->pos_b = data->pos_b;
	vertices->laser_thick = line_thicc;
	vertices++;

	vertices->pos = vec2f(
		a->x + offset.x,
		a->y + offset.y
	);
	vertices->pos_a = data->pos_a;
	vertices->pos_b = data->pos_b;
	vertices->laser_thick = line_thicc;
	vertices++;

	vertices->pos = vec2f(
		b->x + offset.x,
		b->y + offset.y
	);
	vertices->pos_a = data->pos_a;
	vertices->pos_b = data->pos_b;
	vertices->laser_thick = line_thicc;
	vertices++;

	vertices->pos = vec2f(
		b->x - offset.x,
		b->y - offset.y
	);
	vertices->pos_a = data->pos_a;
	vertices->pos_b = data->pos_b;
	vertices->laser_thick = line_thicc;

	ren_add_rect_indices(bro, v);

	bro->vbo.count += RECT_VERT;
}

/**
 * This file contains code adapted from the Nuklear GLFW OpenGL3 demo.
 * Original source: https://github.com/Immediate-Mode-UI/Nuklear/tree/master/demo/glfw_opengl3
 * 
 * The code has been modified to integrate with my custom window library (WA - Window Abstraction),
 * specifically to handle window creation, input, and OpenGL context setup.
 * 
 * Please refer to the original repository for the unmodified code and additional details.
 */

#include "gui/gui.h"
#include "opengl.h"
#include "renderer.h"
#include <stddef.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_KEYSTATE_BASED_INPUT
#include <nuklear.h>

#define NK_WA_TEXT_MAX 256
#define NK_SHADER_VERSION "#version 450 core"
#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

typedef struct nk_wa_vertex
{
    f32 position[2];
    f32 uv[2];
    nk_byte col[4];
} nk_wa_vertex_t;

typedef struct 
{
    struct nk_buffer cmds;
    struct nk_draw_null_texture tex_null;
    u32 vao, vbo, ebo;
    u32 prog;
    u32 attrib_pos;
    u32 attrib_uv;
    u32 attrib_col;
    u32 uniform_tex;
    u32 uniform_proj;
    u32 font_tex;
} wa_device_t;

typedef struct nk_wa
{
    wa_device_t dev;
    i32 w;
    i32 h;
    struct nk_context ctx;
    struct nk_font_atlas atlas;
    struct nk_vec2 fb_scale;
    u32 text[NK_WA_TEXT_MAX];
    i32 text_len;
    struct nk_vec2 scroll;
    f64 last_double_click;
    i32 is_double_click_down;
    struct nk_vec2 double_click_pos;
} nk_wa_t;

static void
nk_wa_dev_create(struct nk_wa* nk)
{
    GLint status;
    wa_device_t* dev = &nk->dev;
    nk_buffer_init_default(&dev->cmds);
    dev->prog = glCreateProgram();
    // u32 vert_shdr = glCreateShader(GL_VERTEX_SHADER);
    // u32 frag_shdr = glCreateShader(GL_FRAGMENT_SHADER);

    u32 vert_shdr = shader_compile("client/src/shaders/nk_vert.glsl", GL_VERTEX_SHADER);
    u32 frag_shdr = shader_compile("client/src/shaders/nk_frag.glsl", GL_FRAGMENT_SHADER);

    glAttachShader(dev->prog, vert_shdr);
    glAttachShader(dev->prog, frag_shdr);
    glLinkProgram(dev->prog);
    glGetProgramiv(dev->prog, GL_LINK_STATUS, &status);
    assert(status == GL_TRUE);

    dev->uniform_tex = glGetUniformLocation(dev->prog, "Texture");
    dev->uniform_proj = glGetUniformLocation(dev->prog, "ProjMtx");
    dev->attrib_pos = glGetAttribLocation(dev->prog, "Position");
    dev->attrib_uv = glGetAttribLocation(dev->prog, "TexCoord");
    dev->attrib_col = glGetAttribLocation(dev->prog, "Color");

    {
        /* buffer setup */
        GLsizei vs = sizeof(struct nk_wa_vertex);
        size_t vp = offsetof(struct nk_wa_vertex, position);
        size_t vt = offsetof(struct nk_wa_vertex, uv);
        size_t vc = offsetof(struct nk_wa_vertex, col);

        glGenBuffers(1, &dev->vbo);
        glGenBuffers(1, &dev->ebo);
        glGenVertexArrays(1, &dev->vao);

        glBindVertexArray(dev->vao);
        glBindBuffer(GL_ARRAY_BUFFER, dev->vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dev->ebo);

        glEnableVertexAttribArray((GLuint)dev->attrib_pos);
        glEnableVertexAttribArray((GLuint)dev->attrib_uv);
        glEnableVertexAttribArray((GLuint)dev->attrib_col);

        glVertexAttribPointer((GLuint)dev->attrib_pos, 2, GL_FLOAT, GL_FALSE, vs, (void*)vp);
        glVertexAttribPointer((GLuint)dev->attrib_uv, 2, GL_FLOAT, GL_FALSE, vs, (void*)vt);
        glVertexAttribPointer((GLuint)dev->attrib_col, 4, GL_UNSIGNED_BYTE, GL_TRUE, vs, (void*)vc);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

static struct nk_wa*
nk_wa_init(void)
{
    struct nk_wa* nk = calloc(1, sizeof(struct nk_wa));

    nk_init_default(&nk->ctx, 0);
    nk_wa_dev_create(nk);

    // TODO: Add scrolling

    return nk;
}

static void
nk_wa_font_stash_begin(nk_wa_t* nk, struct nk_font_atlas** atlas)
{
    nk_font_atlas_init_default(&nk->atlas);
    nk_font_atlas_begin(&nk->atlas);
    *atlas = &nk->atlas;
}

static void
nk_wa_dev_upload_atlas(nk_wa_t* nk, const void* image, i32 w, i32 h)
{
    wa_device_t* dev = &nk->dev;
    glGenTextures(1, &dev->font_tex);
    glBindTexture(GL_TEXTURE_2D, dev->font_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)w, (GLsizei)h, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, image);
}

static void
nk_wa_font_stash_end(nk_wa_t* nk)
{
    const void* image;
    i32 w;
    i32 h;
    image = nk_font_atlas_bake(&nk->atlas, &w, &h, NK_FONT_ATLAS_RGBA32);
    nk_wa_dev_upload_atlas(nk, image, w, h);
    nk_font_atlas_end(&nk->atlas, nk_handle_id((i32)nk->dev.font_tex),
                      &nk->dev.tex_null);
    if (nk->atlas.default_font)
        nk_style_set_font(&nk->ctx, &nk->atlas.default_font->handle);
}

static void
nk_wa_render(nk_wa_t* nk, enum nk_anti_aliasing AA, i32 max_vertex_buffer, i32 max_element_buffer)
{
    wa_device_t* dev = &nk->dev;
    struct nk_buffer vbuf, ebuf;
    GLfloat ortho[4][4] = {
        {2.0f, 0.0f, 0.0f, 0.0f},
        {0.0f,-2.0f, 0.0f, 0.0f},
        {0.0f, 0.0f,-1.0f, 0.0f},
        {-1.0f,1.0f, 0.0f, 1.0f},
    };
    ortho[0][0] /= (GLfloat)nk->w;
    ortho[1][1] /= (GLfloat)nk->h;

    /* setup global state */
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glActiveTexture(GL_TEXTURE0);

    /* setup program */
    glUseProgram(dev->prog);
    glUniform1i(dev->uniform_tex, 0);
    glUniformMatrix4fv(dev->uniform_proj, 1, GL_FALSE, &ortho[0][0]);
    {
        /* convert from command queue into draw list and draw to screen */
        const struct nk_draw_command *cmd;
        void *vertices, *elements;
        nk_size offset = 0;

        /* allocate vertex and element buffer */
        glBindVertexArray(dev->vao);
        glBindBuffer(GL_ARRAY_BUFFER, dev->vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dev->ebo);

        glBufferData(GL_ARRAY_BUFFER, max_vertex_buffer, NULL, GL_STREAM_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, max_element_buffer, NULL, GL_STREAM_DRAW);

        /* load draw vertices & elements directly into vertex + element buffer */
        vertices = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        elements = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
        {
            /* fill convert configuration */
            struct nk_convert_config config;
            static struct nk_draw_vertex_layout_element vertex_layout[] = {
                {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(nk_wa_vertex_t, position)},
                {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(nk_wa_vertex_t, uv)},
                {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(nk_wa_vertex_t, col)},
                {NK_VERTEX_LAYOUT_END}
            };
            memset(&config, 0, sizeof(config));
            config.vertex_layout = vertex_layout;
            config.vertex_size = sizeof(nk_wa_vertex_t);
            config.vertex_alignment = NK_ALIGNOF(nk_wa_vertex_t);
            config.tex_null = dev->tex_null;
            config.circle_segment_count = 22;
            config.curve_segment_count = 22;
            config.arc_segment_count = 22;
            config.global_alpha = 1.0f;
            config.shape_AA = AA;
            config.line_AA = AA;

            /* setup buffers to load vertices and elements */
            nk_buffer_init_fixed(&vbuf, vertices, (size_t)max_vertex_buffer);
            nk_buffer_init_fixed(&ebuf, elements, (size_t)max_element_buffer);
            nk_convert(&nk->ctx, &dev->cmds, &vbuf, &ebuf, &config);
        }
        glUnmapBuffer(GL_ARRAY_BUFFER);
        glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

        /* iterate over and execute each draw command */
        nk_draw_foreach(cmd, &nk->ctx, &dev->cmds)
        {
            if (!cmd->elem_count) continue;
            glBindTexture(GL_TEXTURE_2D, (GLuint)cmd->texture.id);
            glScissor(
                (GLint)(cmd->clip_rect.x * nk->fb_scale.x),
                (GLint)((nk->h - (GLint)(cmd->clip_rect.y + cmd->clip_rect.h)) * nk->fb_scale.y),
                (GLint)(cmd->clip_rect.w * nk->fb_scale.x),
                (GLint)(cmd->clip_rect.h * nk->fb_scale.y));
            glDrawElements(GL_TRIANGLES, (GLsizei)cmd->elem_count, GL_UNSIGNED_SHORT, (const void*) offset);
            offset += cmd->elem_count * sizeof(nk_draw_index);
        }
        nk_clear(&nk->ctx);
        nk_buffer_clear(&dev->cmds);
    }

    /* default OpenGL state */
    glUseProgram(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glDisable(GL_SCISSOR_TEST);
}

static void
nk_wa_new_frame(waapp_t* app)
{
    int i;
    double x, y;
    struct nk_context* ctx = app->nk_ctx;
    wa_window_t* win = app->window;
    wa_state_t* wa_state = wa_window_get_state(win);
    nk_wa_t* nk = app->nk_wa;
    const u8* mb_map = wa_state->mouse_map;

    nk->w = wa_state->window.w;
    nk->h = wa_state->window.h;

    // glfwGetWindowSize(win, &glfw->width, &glfw->height);
    nk->fb_scale.x = 1.0;
    nk->fb_scale.y = 1.0;

    nk_input_begin(ctx);
    for (i = 0; i < nk->text_len; ++i)
        nk_input_unicode(ctx, nk->text[i]);

    // if (glfwGetKey(win, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
    //     glfwGetKey(win, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) {
    //     nk_input_key(ctx, NK_KEY_COPY, glfwGetKey(win, GLFW_KEY_C) == GLFW_PRESS);
    //     nk_input_key(ctx, NK_KEY_PASTE, glfwGetKey(win, GLFW_KEY_V) == GLFW_PRESS);
    //     nk_input_key(ctx, NK_KEY_CUT, glfwGetKey(win, GLFW_KEY_X) == GLFW_PRESS);
    //     nk_input_key(ctx, NK_KEY_TEXT_UNDO, glfwGetKey(win, GLFW_KEY_Z) == GLFW_PRESS);
    //     nk_input_key(ctx, NK_KEY_TEXT_REDO, glfwGetKey(win, GLFW_KEY_R) == GLFW_PRESS);
    //     nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, glfwGetKey(win, GLFW_KEY_LEFT) == GLFW_PRESS);
    //     nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS);
    //     nk_input_key(ctx, NK_KEY_TEXT_LINE_START, glfwGetKey(win, GLFW_KEY_B) == GLFW_PRESS);
    //     nk_input_key(ctx, NK_KEY_TEXT_LINE_END, glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS);
    // } else {
    //     nk_input_key(ctx, NK_KEY_LEFT, glfwGetKey(win, GLFW_KEY_LEFT) == GLFW_PRESS);
    //     nk_input_key(ctx, NK_KEY_RIGHT, glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS);
    //     nk_input_key(ctx, NK_KEY_COPY, 0);
    //     nk_input_key(ctx, NK_KEY_PASTE, 0);
    //     nk_input_key(ctx, NK_KEY_CUT, 0);
    // }

    x = app->mouse.x;
    y = app->mouse.y;
    nk_input_motion(ctx, (i32)x, (i32)y);
// #ifdef NK_GLFW_GL3_MOUSE_GRABBING
//     if (ctx->input.mouse.grabbed) {
//         glfwSetCursorPos(glfw->win, ctx->input.mouse.prev.x, ctx->input.mouse.prev.y);
//         ctx->input.mouse.pos.x = ctx->input.mouse.prev.x;
//         ctx->input.mouse.pos.y = ctx->input.mouse.prev.y;
//     }
// #endif
    nk_input_button(ctx, NK_BUTTON_LEFT, (int)x, (int)y, mb_map[WA_MOUSE_LEFT]);
    nk_input_button(ctx, NK_BUTTON_MIDDLE, (int)x, (int)y, mb_map[WA_MOUSE_MIDDLE]);
    nk_input_button(ctx, NK_BUTTON_RIGHT, (int)x, (int)y, mb_map[WA_MOUSE_RIGHT]);
    nk_input_button(ctx, NK_BUTTON_DOUBLE, (int)nk->double_click_pos.x, (int)nk->double_click_pos.y, nk->is_double_click_down);
    nk_input_scroll(ctx, nk->scroll);
    nk_input_end(&nk->ctx);
    nk->text_len = 0;
    nk->scroll = nk_vec2(0,0);

}

void 
gui_init(waapp_t* app)
{
    app->nk_wa = nk_wa_init();
    struct nk_font_atlas* atlas;

    nk_wa_font_stash_begin(app->nk_wa, &atlas);
    nk_wa_font_stash_end(app->nk_wa);

    app->nk_ctx = &app->nk_wa->ctx;
}

void 
gui_new_frame(waapp_t* app)
{
    nk_wa_new_frame(app);
}

void 
gui_scroll(waapp_t* app, f32 xoff, f32 yoff)
{
    app->nk_wa->scroll.x += xoff;
    app->nk_wa->scroll.y += yoff;
}

void
gui_render(waapp_t* app)
{
    const wa_state_t* state = wa_window_get_state(app->window);

    app->nk_wa->w = state->window.w;
    app->nk_wa->h = state->window.h;

    nk_wa_render(app->nk_wa, NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER,
                 MAX_ELEMENT_BUFFER);
}

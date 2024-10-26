#include "opengl.h"
#include <stdio.h>
#include "gui/gui.h"
#include <nuklear.h>
#include <string.h>

static bool failed = false;

static void
opengl_debug_callback(GLenum src, GLenum type, GLuint id,
                      GLenum severity, GLsizei len, const char* msg,
                      const void* data)
{
    waapp_t* app = (waapp_t*)data;
    wa_window_stop(app->window);
    failed = true;

    fprintf(stderr, "OpenGL Debug Message:\n");
    fprintf(stderr, "\tSource: 0x%x\n", src);
    fprintf(stderr, "\tType: 0x%x\n", type);
    fprintf(stderr, "\tID: %u\n", id);
    fprintf(stderr, "\tSeverity: 0x%x\n", severity);
    fprintf(stderr, "\tMessage (%u): '%s'\n", len, msg);
    fprintf(stderr, "-------------------------------------\n");
}


static void
waapp_enable_debug(waapp_t* app)
{
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(opengl_debug_callback, app);
}

static void
print_gl_version(void)
{
    printf("OpenGL vendor: %s\n",
           glGetString(GL_VENDOR));
    printf("OpenGL renderer: %s\n",
           glGetString(GL_RENDERER));
    printf("OpenGL version: %s\n",
           glGetString(GL_VERSION));
    printf("OpenGL GLSL version: %s\n",
           glGetString(GL_SHADING_LANGUAGE_VERSION));
}

bool
waapp_opengl_init(waapp_t* app)
{
    gladLoadGL();
    waapp_enable_debug(app);
    print_gl_version();

    ren_init(&app->ren);

    wa_state_t* state = wa_window_get_state(app->window);
    ren_viewport(&app->ren, state->window.w, state->window.h);

    gui_init(app);

    ren_set_view(&app->ren, &app->cam);

    return !failed;
}

void
waapp_opengl_cleanup(waapp_t* app)
{
    ren_del(&app->ren);
}

static void
waapp_gui(waapp_t* app)
{
    struct nk_context* ctx = app->nk_ctx;
    struct nk_colorf bg;
    memcpy(&bg, &app->bg_color, sizeof(struct nk_colorf));
    wa_state_t* state = wa_window_get_state(app->window);

    static nk_flags win_flags = 
        NK_WINDOW_BORDER | NK_WINDOW_MINIMIZABLE | 
        NK_WINDOW_MINIMIZED | NK_WINDOW_TITLE | 
        NK_WINDOW_CLOSABLE | NK_WINDOW_MOVABLE;
    const char* window_name = state->window.title;
    // static struct nk_vec2 window_offset = {10, 10};
    static struct nk_rect window_rect = {
        .x = 0,
        .y = 0,
        .w = 300,
        .h = 300
    };

    if (nk_begin(ctx, window_name, window_rect, win_flags))
                 
    {
        if (win_flags & NK_WINDOW_MINIMIZED)
            win_flags ^= NK_WINDOW_MINIMIZED;

        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label(ctx, "Background Color", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 25, 1);
        if (nk_combo_begin_color(ctx, nk_rgb_cf(bg), nk_vec2(nk_widget_width(ctx), 400)))
        {
            nk_layout_row_dynamic(ctx, 200, 1);
            bg = nk_color_picker(ctx, bg, NK_RGBA);
            nk_layout_row_dynamic(ctx, 25, 1);
            bg.r = nk_propertyf(ctx, "#R", 0, bg.r, 1.0f, 0.01f, 0.005f);
            bg.g = nk_propertyf(ctx, "#G", 0, bg.g, 1.0f, 0.01f, 0.005f);
            bg.b = nk_propertyf(ctx, "#B", 0, bg.b, 1.0f, 0.01f, 0.005f);
            bg.a = nk_propertyf(ctx, "#A", 0, bg.a, 1.0f, 0.01f, 0.005f);
            nk_combo_end(ctx);
            memcpy(&app->bg_color, &bg, sizeof(vec4f_t));
        }

        static bool popup = false;

        if (nk_button_label(ctx, "Images"))
            popup = true;

        if (popup)
        {
            if (nk_popup_begin(ctx, NK_POPUP_DYNAMIC, "Images", 0, nk_rect(50, 50, 200, 300)))
            {
                nk_layout_row_dynamic(ctx, 100, 1);
                const ght_t* textures = &app->ren.current_bro->textures;

                GHT_FOREACH(texture_t* texture, textures, 
                    nk_image(ctx, nk_image_id(texture->id));
                );

                nk_layout_row_dynamic(ctx, 20, 1);
                if (nk_button_label(ctx, "Cancel"))
                {
                    popup = false;
                    nk_popup_close(ctx);
                }
                nk_popup_end(ctx);
            } 
            else
                popup = false;
        }

        const char* fullscreen_str = (state->window.state & WA_STATE_FULLSCREEN) ? "Windowed" : "Fullscreen";
        if (nk_button_label(ctx, fullscreen_str))
            wa_window_set_fullscreen(app->window, !(state->window.state & WA_STATE_FULLSCREEN));
    }
    nk_end(ctx);
    if (nk_window_is_hidden(ctx, window_name))
    {
        wa_window_stop(app->window);
        printf("CLOSE!\n");
    }
    // else
    // {
    //     window_rect.x = state->window.w - window_rect.w - window_offset.x;
    //     window_rect.y = window_offset.y;
    //     nk_window_set_position(ctx, window_name, nk_vec2(window_rect.x, window_rect.y));
    // }

    gui_render(app);
}

UNUSED static bool 
rect_in_frustum(waapp_t* app, rect_t* rect)
{
	const wa_state_t* state = wa_window_get_state(app->window);
	rect_t frustum = {
		.pos.x = -app->cam.x,
		.pos.y = -app->cam.y,
		.size.x = state->window.w,
		.size.y = state->window.h,
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

static void 
render_player(waapp_t* app, player_t* player)
{
	player->rect.pos = player->core->rect.pos;
	player->top.pos = player->core->rect.pos;

	ren_draw_rect(&app->ren, &player->rect);
	ren_draw_rect(&app->ren, &player->top);
}

static void
render_projectiles(waapp_t* app)
{
	const ght_t* projectiles = &app->projectiles;
	GHT_FOREACH(projectile_t* proj, projectiles, {
		proj->rect.pos = proj->core->rect.pos;
		ren_draw_rect(&app->ren, &proj->rect);
	});
}

void
waapp_opengl_draw(waapp_t* app)
{
    const vec4f_t* bg_color = &app->bg_color;
    ren_t* ren = &app->ren;
    wa_state_t* state = wa_window_get_state(app->window);

    gui_new_frame(app);

    ren_bind_bro(ren, ren->default_bro);

    ren_clear(&app->ren, bg_color);

    vec3f_t* cam = &app->cam;

    if (app->mouse.x != app->mouse_prev.x || app->mouse.y != app->mouse_prev.y)
    {
        if (state->mouse_map[WA_MOUSE_RIGHT])
        {
            vec2f_t diff = vec2f(
                app->mouse_prev.x - app->mouse.x,
                app->mouse_prev.y - app->mouse.y
            );
            cam->x -= diff.x / ren->scale.x;
            cam->y -= diff.y / ren->scale.y;
            ren_set_view(ren, cam);
        }

        app->mouse_prev = app->mouse;
    }

	render_projectiles(app);

	render_player(app, app->player);

    ren_draw_batch(ren);

	ren_bind_bro(ren, app->line_bro);
	ren_draw_rect(ren, &app->world_border);
	ren_draw_batch(ren);

    waapp_gui(app);
}

u64
gl_sizeof(u32 type)
{
    switch (type)
    {
        case GL_FLOAT:
            return sizeof(GLfloat);
        case GL_INT:
        case GL_UNSIGNED_INT:
            return sizeof(GLuint);
        case GL_UNSIGNED_BYTE:
        case GL_BYTE:
            return sizeof(GLubyte);
        default:
            assert(false);
            return 0;
    }
}

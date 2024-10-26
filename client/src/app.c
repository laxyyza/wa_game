#include "app.h"
#include "opengl.h"
#include <wa_keys.h>
#include <wa_cursor.h>
#include <string.h>
#include "gui/gui.h"
#include "loading.h"

static void
waapp_draw(_WA_UNUSED wa_window_t* window, void* data)
{
    waapp_opengl_draw(data);
}

static void
waapp_event(wa_window_t* window, const wa_event_t* ev, void* data)
{
    waapp_t* app = data;
    wa_state_t* state = wa_window_get_state(window);

    if (ev->type == WA_EVENT_KEYBOARD)
    {
        if (ev->keyboard.pressed)
        {
            if (ev->keyboard.key == WA_KEY_F)
            {
                wa_window_set_fullscreen(window, !(state->window.state & WA_STATE_FULLSCREEN));
            }
            else if (ev->keyboard.key == WA_KEY_V)
            {
                wa_window_vsync(window, !state->window.vsync);
            }
            else if (ev->keyboard.key == WA_KEY_D)
                app->dir.x += 1.0;
            else if (ev->keyboard.key == WA_KEY_A)
                app->dir.x -= 1.0;
            else if (ev->keyboard.key == WA_KEY_W)
                app->dir.y -= 1.0;
            else if (ev->keyboard.key == WA_KEY_S)
                app->dir.y += 1.0;
            else if (ev->keyboard.key == WA_KEY_Q)
            {
                array_pop(&app->rects);
            }
            else if (ev->keyboard.key == WA_KEY_R)
                app->do_rotation = !app->do_rotation;
        }
        else 
        {
            if (ev->keyboard.key == WA_KEY_D || ev->keyboard.key == WA_KEY_A)
                app->dir.x = 0.0;
            if (ev->keyboard.key == WA_KEY_W || ev->keyboard.key == WA_KEY_S)
                app->dir.y = 0.0;
        }
    }
    else if (ev->type == WA_EVENT_RESIZE)
    {
        ren_viewport(&app->ren, ev->resize.w, ev->resize.h);
        // shader_bind(&app->shader);
        // shader_uniform_mat4f(&app->shader, "mvp", &app->ren.proj);
    }
    else if (ev->type == WA_EVENT_POINTER)
    {
        app->mouse.x = ev->pointer.x;
        app->mouse.y = ev->pointer.y;
    }
    else if (ev->type == WA_EVENT_MOUSE_BUTTON)
    {
        if (ev->mouse.button == WA_MOUSE_RIGHT)
        {
            if (ev->mouse.pressed)
            {
                wa_set_cursor_shape(window, WA_CURSOR_ALL_SCROLL);
                app->mouse_prev = app->mouse;
            }
            else
                wa_set_cursor_shape(window, WA_CURSOR_DEFAULT);
        }
    }
    else if (ev->type == WA_EVENT_MOUSE_WHEEL)
    {
        f32 dir = (ev->wheel.value > 0) ? -1.0 : 1.0;

        gui_scroll(app, 0.0, dir);

        if (dir > 0)
        {
            app->ren.scale.x += 0.1;
            app->ren.scale.y += 0.1;
        }
        else
        {
            app->ren.scale.x -= 0.1;
            app->ren.scale.y -= 0.1;

            if (app->ren.scale.x < 0.1)
                app->ren.scale.x = 0.1;
            if (app->ren.scale.y < 0.1)
                app->ren.scale.y = 0.1;
        }
        ren_set_scale(&app->ren, &app->ren.scale);
    }
}

static void 
waapp_close(_WA_UNUSED wa_window_t* window, _WA_UNUSED void* data)
{

}

i32 
waapp_init(waapp_t* app, i32 argc, const char** argv)
{
    const char* title = "WA OpenGL";
    const char* app_id = "wa_opengl";
    i32 w = 640;
    i32 h = 480;
    bool fullscreen = false;
    wa_state_t* state;

    if (argc >= 2 && strcmp(argv[1], "fullscreen") == 0)
        fullscreen = true;

    if ((app->window = wa_window_create(title, w, h, fullscreen)) == NULL)
        return -1;

    state = wa_window_get_state(app->window);
    state->window.wayland.app_id = app_id;
    state->user_data = app;
    state->callbacks.draw = waapp_draw;
    state->callbacks.event = waapp_event;
    state->callbacks.close = waapp_close;

    app->bg_color = rgba(0x111111FF);

    // if (argc == 2 && fullscreen == false)
    //     app->texture_path = argv[1];
    // else if (argc == 3)
    //     app->texture_path = argv[2];
    // else 
    //     app->texture_path = "res/test.png";
    //
    if (waapp_opengl_init(app) == false)
    {
        wa_window_delete(app->window);
        return -1;
    }

    array_init(&app->rects, sizeof(rect_t), 8);

    vec2i_t grid_size = {200, 200};

	u32 textures_count;
#ifdef __linux__
	#define PATH_LEN 1024
	char path[PATH_LEN];
	if (argc == 1)
	{
		const char* homepath = getenv("HOME");
		snprintf(path, 1024, "%s/Pictures/Wallpapers", homepath);
	}
	else
		strncpy(path, argv[1], 1024);
#else
	const char* path = argv[1];
#endif
    texture_t** textures = waapp_load_textures_threaded(path, &textures_count);

    rect_t* rect;
    u32 color = 0xFFFFFFFF;
    u32 ti = 1;

    vec2f_t rect_size = {300, 150};
    vec2f_t rect_gaps = {1, 1};

	for (i32 y = 0; y <= grid_size.y; y++)
    {
		for (i32 x = 0; x <= grid_size.x; x++)
        {
            rect = array_add_into(&app->rects);
            rect_init(
                rect,
                vec2f(
                    (rect_size.x + rect_gaps.x) * x, 
                    (rect_size.y + rect_gaps.y) * y
                ),
                rect_size,
                color,
                textures[ti]
            );
            ti++;
            if (ti >= textures_count)
                ti = 0;
        }
    }

    // Non texture testing.
    rect = array_idx(&app->rects, 2);
    rect->texture = NULL;
    rect->color = rgba(0xFF00FFFF);

    return 0;
}

void 
waapp_run(waapp_t* app)
{
    wa_window_mainloop(app->window);
}

void 
waapp_cleanup(waapp_t* app)
{
    array_del(&app->rects);
    waapp_opengl_cleanup(app);
    wa_window_delete(app->window);
}

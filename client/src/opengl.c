#define _GNU_SOURCE
#include "opengl.h"
#include <stdio.h>
#include "gui/gui.h"
#include <nuklear.h>
#include <string.h>
#include "client_net.h"

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
    info("OpenGL vendor: %s\n",
           glGetString(GL_VENDOR));
    info("OpenGL renderer: %s\n",
           glGetString(GL_RENDERER));
    info("OpenGL version: %s\n",
           glGetString(GL_VERSION));
    info("OpenGL GLSL version: %s\n",
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

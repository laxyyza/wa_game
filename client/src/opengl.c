#define _GNU_SOURCE
#include "opengl.h"
#include <stdio.h>
#include "gui/gui.h"
#include <nuklear.h>
#include <string.h>
#include "client_net.h"

static bool failed = false;

static const char*
opengl_severity_str(GLenum sev)
{
	switch (sev) 
	{
		case GL_DEBUG_SEVERITY_LOW:
			return "GL_DEBUG_SEVERITY_LOW";
		case GL_DEBUG_SEVERITY_MEDIUM:
			return "GL_DEBUG_SEVERITY_MEDIUM";
		case GL_DEBUG_SEVERITY_HIGH:
			return "GL_DEBUG_SEVERITY_HIGH";
		case GL_DEBUG_SEVERITY_NOTIFICATION:
			return "GL_DEBUG_SEVERITY_NOTIFICATION";
		default:
			return "Unknown";
	}
}

static const char*
opengl_debug_type_str(GLenum type)
{
	switch (type)
	{
		case GL_DEBUG_TYPE_ERROR:
			return "GL_DEBUG_TYPE_ERROR";
		case GL_DEBUG_TYPE_OTHER:
			return "GL_DEBUG_TYPE_OTHER";
		case GL_DEBUG_TYPE_MARKER:
			return "GL_DEBUG_TYPE_MARKER";
		case GL_DEBUG_TYPE_POP_GROUP:
			return "GL_DEBUG_TYPE_POP_GROUP";
		case GL_DEBUG_TYPE_PERFORMANCE:
			return "GL_DEBUG_TYPE_PERFORMANCE";
		case GL_DEBUG_TYPE_PORTABILITY:
			return "GL_DEBUG_TYPE_PORTABILITY";
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
			return "GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR";
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
			return "GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR";
		default:
			return "Unknown";
	}
}

static const char*
opengl_debug_source_str(GLenum src)
{
	switch (src)
	{
		case GL_DEBUG_SOURCE_OTHER:
			return "GL_DEBUG_SOURCE_OTHER";
		case GL_DEBUG_SOURCE_THIRD_PARTY:
			return "GL_DEBUG_SOURCE_THIRD_PARTY";
		case GL_DEBUG_SOURCE_API:
			return "GL_DEBUG_SOURCE_API";
		case GL_DEBUG_SOURCE_APPLICATION:
			return "GL_DEBUG_SOURCE_APPLICATION";
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
			return "GL_DEBUG_SOURCE_WINDOW_SYSTEM";
		case GL_DEBUG_SOURCE_SHADER_COMPILER:
			return "GL_DEBUG_SOURCE_SHADER_COMPILER";
		default:
			return "Unknown";
	}
}

static void
opengl_debug_callback(GLenum src, GLenum type, GLuint id,
                      GLenum severity, UNUSED GLsizei len, const char* msg,
                      UNUSED const void* data)
{
	if (type == GL_DEBUG_TYPE_ERROR && severity == GL_DEBUG_SEVERITY_HIGH)
		failed = true;

    fprintf(stderr, "OpenGL Debug Message:\n");
    fprintf(stderr, "\tSource:\t\t0x%x (%s)\n", src, opengl_debug_source_str(src));
    fprintf(stderr, "\tType:\t\t0x%x (%s)\n", type, opengl_debug_type_str(type));
    fprintf(stderr, "\tID:\t\t%u\n", id);
    fprintf(stderr, "\tSeverity:\t0x%x (%s)\n\n", severity, opengl_severity_str(severity));
    fprintf(stderr, "\tMessage:\t'%s'\n", msg);
    fprintf(stderr, "-------------------------------------\n");
}


static void
waapp_enable_debug(waapp_t* app)
{
	if (app->disable_debug)
		return;

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
	if (app->headless)
		return true;

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

#include "shader.h"
#include "ght.h"
#include "opengl.h"
#include <stdio.h>
#include <stdlib.h>

static const char*
shader_readfile(const char* path)
{
    char* buf;
    FILE* f = fopen(path, "r");
    if (f == NULL)
    {
        perror("fopen");
        return NULL;
    }
    u64 size = file_size(f);
    buf = malloc(size + 1);
    buf[size] = 0x00;
    fread(buf, 1, size, f);
    fclose(f);
    return buf;
}

u32
shader_compile(const char* path, GLenum type)
{
    u32 shader = glCreateShader(type);
    const char* src = shader_readfile(path);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    i32 ret;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ret);
    if (!ret)
    {
        char log[512];
        glGetShaderInfoLog(shader, 512, NULL, log);
        error("ERROR:compile_shader of type: %s:\n",
                (type == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT");
        error("\t%s\n", log);
    }
    free((void*)src);
    return shader;
}

void 
shader_init(shader_t* shader, const char* vertex_path, const char* fragment_path)
{
    u32 vs = shader_compile(vertex_path, GL_VERTEX_SHADER);
    u32 fs = shader_compile(fragment_path, GL_FRAGMENT_SHADER);

    shader->id = glCreateProgram();
    glAttachShader(shader->id, vs);
    glAttachShader(shader->id, fs);
    glLinkProgram(shader->id);

    glDeleteShader(vs);
    glDeleteShader(fs);

    shader_bind(shader);

    ght_init(&shader->location_cache, 10, NULL);
}

void 
shader_del(shader_t* shader)
{
    glDeleteProgram(shader->id);
    ght_destroy(&shader->location_cache);
}

void 
shader_bind(const shader_t* shader)
{
    glUseProgram(shader->id);
}

void 
shader_unbind(void)
{
    glUseProgram(0);
}

static i32
shader_uniform_location(shader_t* shader, const char* name)
{
    i64 location;
    void* ret;
    if ((ret = ght_get(&shader->location_cache, 
                       ght_hashstr(name))) != NULL)
    {
        location = (i64)ret;
        return location;
    }

    location = glGetUniformLocation(shader->id, name);
    ght_insert(&shader->location_cache, 
               ght_hashstr(name), 
               (void*)location);
    return location;
}

void 
shader_uniform4f(shader_t* shader, const char* name, 
                 f32 x, f32 y, f32 z, f32 w)
{
    i32 location = shader_uniform_location(shader, name);
    glUniform4f(location, x, y, z, w);
}

void 
shader_uniform_vec4f(shader_t* shader, 
                     const char* name,
                     const vec4f_t* vec)
{
    i32 location = shader_uniform_location(shader, name);
    glUniform4fv(location, 1, (f32*)vec);
}

void 
shader_uniform1i(shader_t* shader, 
                 const char* name, 
                 i32 value)
{
    i32 location = shader_uniform_location(shader, name);
    glUniform1i(location, value);
}

void 
shader_uniform1iv(shader_t* shader, 
                 const char* name, 
                 i32* value,
                 u32 count)
{
    i32 location = shader_uniform_location(shader, name);
    glUniform1iv(location, count, value);
}

void 
shader_uniform_mat4f(shader_t* shader, 
                      const char* name, 
                      const mat4_t* mat4)
{
    i32 location = shader_uniform_location(shader, name);
    glUniformMatrix4fv(location, 1, GL_FALSE, 
                       (f32*)mat4->m);
}

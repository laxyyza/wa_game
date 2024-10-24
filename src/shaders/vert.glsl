#version 450 core

layout(location = 0) in vec4 pos;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 text_coords;
layout(location = 3) in float texture_id;

uniform mat4 mvp;

out vec4 v_color;
out vec2 v_texcoords;
out float v_tid;

void main()
{
    v_color = color;
    v_texcoords = text_coords;
    v_tid = texture_id;
    gl_Position = mvp * pos;
}

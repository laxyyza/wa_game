#version 450 core

layout(location = 0) in vec4 pos;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 text_coords;
layout(location = 3) in float texture_id;

out vec4 v_pos;
out vec4 v_glpos;

void main()
{
    gl_Position = pos;
	v_glpos = gl_Position;
	v_pos = pos;
}

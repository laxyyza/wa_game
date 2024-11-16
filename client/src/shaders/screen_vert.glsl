#version 450 core

layout(location = 0) in vec2 pos;
layout(location = 1) in vec4 color;

uniform mat4 proj;

out vec4 v_color;

void main()
{
	gl_Position = proj * vec4(pos, 0.0, 1.0);
	v_color = color;
}

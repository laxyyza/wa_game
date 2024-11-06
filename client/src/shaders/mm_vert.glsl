#version 450 core

layout(location = 0) in vec4 pos;

out vec4 v_glpos;

void main()
{
	v_glpos = pos;
    gl_Position = pos;
}

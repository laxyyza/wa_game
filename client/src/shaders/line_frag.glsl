#version 450 core

layout(location = 0) out vec4 out_color;

in vec4 v_color;

void main()
{
    out_color = v_color;
}

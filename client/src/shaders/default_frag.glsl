#version 450 core

layout(location = 0) out vec4 out_color;

in vec4 v_color;
in vec2 v_texcoords;
in float v_tid;

uniform sampler2D u_textures[32];

void main()
{
    int idx = int(v_tid);
    if (idx == -1)
        out_color = v_color;
    else
        out_color = texture(u_textures[idx], v_texcoords);
}

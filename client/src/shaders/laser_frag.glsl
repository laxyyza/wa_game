#version 450 core

layout(location = 0) out vec4 out_color;

uniform float scale;

in vec2 v_pos;
in vec2 v_pos_a;
in vec2 v_pos_b;
in float line_thick;

float dist_to_line(vec2 p, vec2 a, vec2 b)
{
    vec2 pa = p - a;
    vec2 ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h);
}

void main()
{
	float line_thiccness = line_thick * scale;

	float d = 0.006 / dist_to_line(v_pos, v_pos_a, v_pos_b) * scale;

	vec4 color = vec4(1.0, 0.15, 0.15, 1.0);

	out_color = color * d;
}

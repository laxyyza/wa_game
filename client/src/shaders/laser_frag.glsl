#version 450 core

layout(location = 0) out vec4 out_color;

uniform float scale;

in vec2 v_pos;
in vec2 v_pos_a;
in vec2 v_pos_b;

void main()
{
	if (v_pos_a == v_pos_b)
		discard;

	float line_thiccness = 0.025 * scale;

	vec2 line_vec = v_pos_b - v_pos_a;

	vec2 frag_vec = v_pos - v_pos_a;

	float proj_len = clamp(dot(frag_vec, line_vec) / dot(line_vec, line_vec), 0.0, 1.0);
	vec2 closest_point = v_pos_a + proj_len * line_vec;

	float dist = length(v_pos - closest_point);

	if (dist < line_thiccness)
	{
		float d = 0.01 / dist * scale;
		float div = 0.15;
		out_color = vec4(d, d * div, d * div, d);
	}
	else
		discard;
}

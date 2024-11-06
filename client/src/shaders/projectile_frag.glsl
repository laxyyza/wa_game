#version 450 core

layout(location = 0) out vec4 out_color;

uniform float time;
uniform float trail_duration;
uniform float trail_faderate;

in vec2 v_pos;
in vec2 v_velocity;
in vec2 v_projectile_pos;

void main()
{
	float time_since_proj = time - length(v_pos - v_projectile_pos) / length(v_velocity);

	float alpha = max(0.0, 1.0 - time_since_proj / trail_duration) * exp(-time_since_proj * trail_faderate);

	out_color = vec4(1.0, 0.0, 0.0, alpha);
}

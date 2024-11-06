#version 450 core

layout(location = 0) in vec4 pos;
layout(location = 1) in vec4 projectile_pos;
layout(location = 2) in vec2 velocity;

uniform mat4 mvp;

out vec2 v_pos;
out vec2 v_velocity;
out vec2 v_projectile_pos;

void main()
{
	v_velocity = velocity;
	v_projectile_pos = (mvp * projectile_pos).xy;
    gl_Position = mvp * pos;
	v_pos = gl_Position.xy;
}

#version 450 core

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 pos_a;
layout(location = 2) in vec2 pos_b;
layout(location = 3) in float in_laser_thick;

uniform mat4 mvp;

out vec2 v_pos;
out vec2 v_pos_a;
out vec2 v_pos_b;
out float line_thick;

void main()
{
	line_thick = in_laser_thick;
	v_pos_a = (mvp * vec4(pos_a, 0.0, 1.0)).xy;
	v_pos_b = (mvp * vec4(pos_b, 0.0, 1.0)).xy;
	gl_Position = mvp * vec4(pos, 0.0, 1.0);
	v_pos = gl_Position.xy;
}

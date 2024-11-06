#version 450 core

layout(location = 0) out vec4 out_color;

in vec4 v_glpos;

uniform vec2 res;
uniform float time;

vec3 palette(float t)
{
	vec3 a = vec3(0.4, 0.1, 0.8);
	vec3 b = vec3(0.5, 0.1, 0.4);
	vec3 c = vec3(1.0, 0.8, 3.0);
	vec3 d = vec3(0.063, 0.616, 0.757);

	return a + b * cos(6.28318 * (c * t + d));
}

void main()
{
	vec2 uv = v_glpos.xy;
	uv.x *= res.x / res.y;

	vec2 uv0 = uv;
	vec3 final = vec3(0.0);

	for (float i = 0.0; i < 10.0; i++)
	{
		float d = length(uv);

		vec3 col = palette(length(uv0) + time * 0.1);

		d = sin(d * 8.0 + time) / 8.0;
		d = abs(d);

		float sd = 0.0008;

		d = sd / d;

		final += col * d;
	}

	out_color = vec4(final, 1.0);
}

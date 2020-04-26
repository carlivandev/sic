//? #version 410 core

struct Light
{
	vec4 m_position_and_unused;
	vec4 m_color_and_intensity;
};

layout(std140) uniform block_lights
{
	uniform float num_lights;
	uniform Light lights[100];
};
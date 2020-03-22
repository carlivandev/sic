#version 330 core

in vec2 texcoord;

layout(location = 0) out vec4 out_color;

uniform sampler2D uniform_texture;

layout(std140) uniform block_test
{
	vec3 first_value;
	vec3 second_value;
};

void main()
{
    out_color = texture(uniform_texture, texcoord.xy);
	//out_color = vec4(second_value.xyz, 1);
}
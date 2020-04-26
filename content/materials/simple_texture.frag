#version 410 core

in vec2 texcoord;

layout(location = 0) out vec4 out_color;

uniform sampler2D uniform_texture;

void main()
{
    out_color = texture(uniform_texture, texcoord.xy);
	//out_color = vec4(first_value.xyz, 1);
}
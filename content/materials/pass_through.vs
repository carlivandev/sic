#version 330 core

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_texcoord;

out vec2 texcoord;

void main()
{
	texcoord = in_texcoord;
    gl_Position = vec4(in_position.x, in_position.y, 0.0f, 1.0f);
}
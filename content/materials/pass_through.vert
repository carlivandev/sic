#version 410 core

layout(location = 0) in vec2 in_position2D;
layout(location = 1) in vec2 in_texcoord;

out vec2 texcoord;

void main()
{
	texcoord = in_texcoord;
    gl_Position = vec4(in_position2D.x, in_position2D.y, 0.0f, 1.0f);
}
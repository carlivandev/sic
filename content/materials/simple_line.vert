#version 330 core

layout(location = 0) in vec3 in_position3D;
layout(location = 1) in vec4 in_color;

layout(std140) uniform block_view
{
	uniform mat4 view_matrix;
	uniform mat4 projection_matrix;
	uniform mat4 view_projection_matrix;
};

out vec4 color;

void main()
{
    gl_Position = view_projection_matrix * vec4(in_position3D, 1.0f);
	color = in_color;
}
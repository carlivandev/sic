//? #version 410 core

layout(std140) uniform block_view
{
	uniform mat4 view_matrix;
	uniform mat4 projection_matrix;
	uniform mat4 view_projection_matrix;
};
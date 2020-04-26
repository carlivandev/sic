#version 330 core

//! #include "includes/block_view.glsl"

layout(location = 0) in vec3 in_position3D;
layout(location = 1) in vec4 in_color;

out vec4 color;

void main()
{
    gl_Position = view_projection_matrix * vec4(in_position3D, 1.0f);
	color = in_color;
}
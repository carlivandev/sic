#version 410 core
//! #include "includes/ui_vert_header.glsl"

void main()
{
	texcoord = in_texcoord;
    gl_Position = vec4(in_position2D, 0.0f, 1.0f);
}
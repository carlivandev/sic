#version 410 core
//! #include "includes/ui_vert_header.glsl"

uniform vec4 lefttop_rightbottom_packed;

void main()
{
	texcoord = in_texcoord;

    gl_Position = vec4(get_ui_position(lefttop_rightbottom_packed.xy, lefttop_rightbottom_packed.zw), 0.0f, 1.0f);
}
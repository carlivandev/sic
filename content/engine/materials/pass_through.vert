#version 410 core
//! #include "includes/ui_vert_header.glsl"

uniform vec4 topleft_bottomright_packed;

void main()
{
	texcoord = in_texcoord;

	vec2 topleft = (topleft_bottomright_packed.xy * 2.0f) - vec2(1.0f, 1.0f);
	vec2 bottomright = (topleft_bottomright_packed.zw * 2.0f) - vec2(1.0f, 1.0f);

	vec2 size = bottomright - topleft;
	vec2 pos =  topleft + (in_position2D * size);
    gl_Position = vec4(in_position2D.x, in_position2D.y, 0.0f, 1.0f);
}
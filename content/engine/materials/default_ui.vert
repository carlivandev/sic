#version 410 core
//! #include "includes/ui_vert_header.glsl"

//out vec2 out_pos;

uniform vec4 lefttop_rightbottom_packed;

void main()
{
	texcoord = in_texcoord;

	vec2 lefttop = (lefttop_rightbottom_packed.xy * 2.0f) - vec2(1.0f, 1.0f);
	vec2 rightbottom = (lefttop_rightbottom_packed.zw * 2.0f) - vec2(1.0f, 1.0f);

	//lefttop = vec2(-1.0f, -1.0f);
	//rightbottom = vec2(1.0f, 1.0f);
	
	vec2 position_remapped = (in_position2D + vec2(1.0f, 1.0f)) * 0.5f;
	position_remapped.y = 1.0f - position_remapped.y;

	vec2 size = rightbottom - lefttop;
	vec2 pos =  lefttop + (position_remapped * size);
    gl_Position = vec4(pos.x, pos.y * -1.0f, 0.0f, 1.0f);
	//out_pos = position_remapped;
}
//? #version 410 core

layout(location = 0) in vec2 in_position2D;
layout(location = 1) in vec2 in_texcoord;

out vec2 texcoord;

vec2 get_ui_position(vec2 in_lefttop, vec2 in_rightbottom)
{
	vec2 lefttop = (in_lefttop * 2.0f) - vec2(1.0f, 1.0f);
	vec2 rightbottom = (in_rightbottom * 2.0f) - vec2(1.0f, 1.0f);

	vec2 position_remapped = (in_position2D + vec2(1.0f, 1.0f)) * 0.5f;
	position_remapped.y = 1.0f - position_remapped.y;

	vec2 size = rightbottom - lefttop;
	vec2 pos =  lefttop + (position_remapped * size);
    
	return vec2(pos.x, pos.y * -1.0f);
}
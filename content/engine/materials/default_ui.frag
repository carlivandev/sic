#version 410 core
//! #include "includes/ui_frag_header.glsl"

//in vec2 out_pos;

uniform vec4 topleft_bottomright_packed;
uniform sampler2D uniform_texture;

void main()
{
    vec4 sample_result = texture(uniform_texture, texcoord.xy);
    out_color = sample_result;
}
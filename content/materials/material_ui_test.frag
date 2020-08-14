#version 410 core
//! #include "../engine/materials/includes/ui_frag_header.glsl"

uniform sampler2D uniform_texture;
uniform vec4 topleft_bottomright_packed;

void main()
{
    vec4 sample_result = texture(uniform_texture, texcoord.xy);
    out_color = sample_result;
}
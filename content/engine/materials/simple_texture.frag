#version 410 core
//! #include "includes/ui_frag_header.glsl"

uniform sampler2D uniform_texture;

void main()
{
    out_color = texture(uniform_texture, texcoord.xy);
}
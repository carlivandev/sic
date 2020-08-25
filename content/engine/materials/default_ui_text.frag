#version 410 core
//! #include "includes/ui_frag_header.glsl"

uniform sampler2D msdf;
uniform vec4 offset_and_size;
//uniform float pxRange;
//uniform vec4 bgColor;
//uniform vec4 fgColor;

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

void main()
{
    float pxRange = 32.0f;
    vec4 bgColor = vec4(1.0f, 0.0f, 0.0f, 0.5f);
    vec4 fgColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);

    vec2 sample_coord = offset_and_size.xy + (offset_and_size.zw * texcoord);

    vec2 texture_size = vec2(textureSize(msdf, 0));

    vec2 msdfUnit = pxRange/vec2(texture_size);
    vec3 sample_result = texture(msdf, sample_coord).rgb;

    float sigDist = median(sample_result.r, sample_result.g, sample_result.b) - 0.5;
    sigDist *= dot(msdfUnit, 0.5/fwidth(sample_coord));
    float opacity = clamp(sigDist + 0.5, 0.0, 1.0);
    out_color = mix(bgColor, fgColor, opacity);
}
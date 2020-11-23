#version 410 core
//?#extension GL_ARB_bindless_texture : require
//?#extension GL_NV_gpu_shader5 : require

//! #include "includes/ui_frag_header.glsl"
//! #include "includes/instancing_common.glsl"

flat in int frag_instanceID;

float median(float r, float g, float b)
{
    return max(min(r, g), min(max(r, g), b));
}

const float width = 0.15f;
const float edge = 0.8f;

void main()
{
    samplerBuffer instance_data_texture_sampler = samplerBuffer(instance_data_texture);

	int instance_data_begin = get_instance_data_begin(frag_instanceID);
	int instance_data_it = 1; //for now assume that leftop_rightbottom is at 0 always, so skip to 1

    sampler2D msdf = read_sampler2D(instance_data_texture_sampler, instance_data_begin, instance_data_it);
    vec4 offset_and_size = read_vec4(instance_data_texture_sampler, instance_data_begin, instance_data_it);
    vec4 fg_color = read_vec4(instance_data_texture_sampler, instance_data_begin, instance_data_it);
    vec4 bg_color = read_vec4(instance_data_texture_sampler, instance_data_begin, instance_data_it);

    //instancing setup end

    vec2 sample_coord = offset_and_size.xy + (offset_and_size.zw * texcoord);
    vec3 sample_result = texture(msdf, sample_coord).rgb;

    float med = median(sample_result.r, sample_result.g, sample_result.b);
    float sigDist = 1.0 - med;
    float opacity = 1.0 - smoothstep(width, width + edge, sigDist);

    out_color = mix(bg_color, fg_color, opacity);

}
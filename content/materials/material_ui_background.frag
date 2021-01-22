#version 410 core
//?#extension GL_ARB_bindless_texture : require
//?#extension GL_NV_gpu_shader5 : require

//! #include "../engine/materials/includes/ui_frag_header.glsl"
//! #include "../engine/materials/includes/instancing_common.glsl"

flat in int frag_instanceID;

void main()
{
    samplerBuffer instance_data_texture_sampler = samplerBuffer(instance_data_texture);

	int instance_data_begin = get_instance_data_begin(frag_instanceID);
	int instance_data_it = 1; //for now assume that leftop_rightbottom is at 0 always, so skip to 1

    vec4 center_color = read_vec4(instance_data_texture_sampler, instance_data_begin, instance_data_it);
    vec4 outer_color = read_vec4(instance_data_texture_sampler, instance_data_begin, instance_data_it);

    vec2 circle_center = texcoord.xy - vec2(0.5f, 0.5f);

    float len = length(circle_center);
    float t = 1.0f - mix(0.0f, 1.0f, len * 2.0f);

    out_color = mix(outer_color, center_color, t);
}
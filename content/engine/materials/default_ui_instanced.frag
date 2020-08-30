#version 410 core
//?#extension GL_ARB_bindless_texture : require
//?#extension GL_NV_gpu_shader5 : require

//! #include "includes/ui_frag_header.glsl"
//! #include "includes/instancing_common.glsl"

flat in int frag_instanceID;

void main()
{
    samplerBuffer instance_data_texture_sampler = samplerBuffer(instance_data_texture);

	int instance_data_begin = get_instance_data_begin(frag_instanceID);
	int instance_data_it = 1; //for now assume that leftop_rightbottom is at 0 always, so skip to 1

    sampler2D uniform_texture = read_sampler2D(instance_data_texture_sampler, instance_data_begin, instance_data_it);

    vec4 sample_result = texture(uniform_texture, texcoord.xy);
    out_color = sample_result;
}
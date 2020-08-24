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
	int instance_data_it = 0;

    sampler2D uniform_texture = read_sampler2D(instance_data_texture_sampler, instance_data_begin, instance_data_it);

    vec4 sample_result = texture(uniform_texture, texcoord.xy);
    out_color = sample_result;
}
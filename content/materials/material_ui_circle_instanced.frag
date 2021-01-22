#version 410 core
//?#extension GL_ARB_bindless_texture : require
//?#extension GL_NV_gpu_shader5 : require

//! #include "../engine/materials/includes/ui_frag_header.glsl"
//! #include "../engine/materials/includes/instancing_common.glsl"

flat in int frag_instanceID;

float draw_circle(vec2 coord)
{
    float len = length(coord);
    return 1.0f - smoothstep(0.97f, 1.0f, len * 2.0f);
}

void main()
{
    samplerBuffer instance_data_texture_sampler = samplerBuffer(instance_data_texture);

	int instance_data_begin = get_instance_data_begin(frag_instanceID);
	int instance_data_it = 1; //for now assume that leftop_rightbottom is at 0 always, so skip to 1

    vec4 tint = read_vec4(instance_data_texture_sampler, instance_data_begin, instance_data_it);

    vec2 circle_center = texcoord.xy - vec2(0.5f, 0.5f);

    float circle = draw_circle(texcoord.xy - vec2(0.5f, 0.5f));

    out_color = vec4(tint.rgb, circle);
}
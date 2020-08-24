//? #version 410 core
//?#extension GL_ARB_bindless_texture : require
//?#extension GL_NV_gpu_shader5 : require

//! #include "includes/ui_vert_header.glsl"
//! #include "includes/instancing_common.glsl"

flat out int frag_instanceID;

void main()
{
	samplerBuffer instance_data_texture_sampler = samplerBuffer(instance_data_texture);

	int instance_data_begin = get_instance_data_begin(gl_InstanceID);
	int instance_data_it = 0;

    sampler2D uniform_texture = read_sampler2D(instance_data_texture_sampler, instance_data_begin, instance_data_it);
    vec4 lefttop_rightbottom_packed = read_vec4(instance_data_texture_sampler, instance_data_begin, instance_data_it);
	
    gl_Position = vec4(get_ui_position(lefttop_rightbottom_packed.xy, lefttop_rightbottom_packed.zw), 0.0f, 1.0f);
	texcoord = in_texcoord;
	frag_instanceID = gl_InstanceID;
}
//? #version 410 core
//? #extension GL_ARB_bindless_texture : require
//? #extension GL_NV_gpu_shader5 : require

//! #include "../engine/materials/includes/mesh_frag_header.glsl"
//! #include "../engine/materials/includes/lighting_common.glsl"
//! #include "../engine/materials/includes/instancing_common.glsl"

flat in int frag_instanceID;

void main()
{
	sampler2D instance_data_texture_sampler = sampler2D(instance_data_texture);

	int instance_data_begin = get_instance_data_begin(frag_instanceID);
	int instance_data_it = 0;

	mat4 mvp = read_mat4(instance_data_texture_sampler, instance_data_begin, instance_data_it);
	mat4 model_matrix = read_mat4(instance_data_texture_sampler, instance_data_begin, instance_data_it);
	sampler2D albedo_sampler = read_sampler2D(instance_data_texture_sampler, instance_data_begin, instance_data_it);

	vec3 material_diffuse_color = texture(albedo_sampler, texcoord ).rgb;
	color = (calculate_light_for_fragment(material_diffuse_color, position_worldspace, normal_cameraspace, eye_direction_cameraspace) * 3.f);
}
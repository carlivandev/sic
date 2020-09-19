//? #version 410 core
//? #extension GL_ARB_bindless_texture : require
//? #extension GL_NV_gpu_shader5 : require

//! #include "../../engine/materials/includes/mesh_frag_header.glsl"
//! #include "../../engine/materials/includes/lighting_common.glsl"
//! #include "../../engine/materials/includes/instancing_common.glsl"

flat in int frag_instanceID;
in mat3 frag_TBN;
void main()
{
	samplerBuffer instance_data_texture_sampler = samplerBuffer(instance_data_texture);

	int instance_data_begin = get_instance_data_begin(frag_instanceID);
	int instance_data_it = 0;

	mat4 mvp = read_mat4(instance_data_texture_sampler, instance_data_begin, instance_data_it);
	mat4 model_matrix = read_mat4(instance_data_texture_sampler, instance_data_begin, instance_data_it);
	sampler2D albedo_sampler = read_sampler2D(instance_data_texture_sampler, instance_data_begin, instance_data_it);
	sampler2D normal_sampler = read_sampler2D(instance_data_texture_sampler, instance_data_begin, instance_data_it);
	sampler2D ao_sampler = read_sampler2D(instance_data_texture_sampler, instance_data_begin, instance_data_it);
	sampler2D r_sampler = read_sampler2D(instance_data_texture_sampler, instance_data_begin, instance_data_it);
	sampler2D m_sampler = read_sampler2D(instance_data_texture_sampler, instance_data_begin, instance_data_it);

	vec3 albedo = texture(albedo_sampler, texcoord ).rgb;
	
	vec3 normal = texture(normal_sampler, texcoord ).rgb;
	normal = normal * 2.0 - 1.0;   
	normal = normalize(frag_TBN * normal); 

	float ao = texture(ao_sampler, texcoord ).r;
	float r = texture(r_sampler, texcoord ).r;
	float m = texture(m_sampler, texcoord ).r;

	float fakeFog = 1.0f - (gl_FragCoord.z / gl_FragCoord.w) * 0.00005f;
	vec3 fogColor = vec3(0.0f, 0.05f, 0.05f);
	fogColor *= 1.0f - fakeFog;
	color = calculate_light_for_fragment_pbr(albedo, normal, m, r, ao, position_worldspace, eye_direction_cameraspace) * clamp(fakeFog, 0.0f, 1.0f);
	color += fogColor;
}
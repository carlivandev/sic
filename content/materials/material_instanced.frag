#version 410 core

//! #include "../engine/materials/includes/mesh_frag_header.glsl"
//! #include "../engine/materials/includes/lighting_common.glsl"

#extension GL_ARB_bindless_texture : require
#extension GL_NV_gpu_shader5 : require

struct Instance_data
{
	mat4 MVP;
	mat4 model_matrix;
	uint64_t tex_albedo;
};

layout(std140) uniform block_instance
{
	Instance_data instance_data[256];
};

flat in int frag_instanceID;

void main()
{
	sampler2D albedo_sampler = sampler2D(instance_data[frag_instanceID].tex_albedo);
	vec3 material_diffuse_color = texture(albedo_sampler, texcoord ).rgb;
	color = (calculate_light_for_fragment(material_diffuse_color, position_worldspace, normal_cameraspace, eye_direction_cameraspace) * 3.f);
}
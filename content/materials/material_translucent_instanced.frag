#version 330 core

//! #include "../engine/materials/includes/mesh_translucent_frag_header.glsl"
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

void main()
{
	vec3 material_diffuse_color = vec3(0.0f, 0.0f, 1.0f);
	//color = vec4(calculate_light_for_fragment(material_diffuse_color, position_worldspace, normal_cameraspace, eye_direction_cameraspace), 1.0f);
	color = vec4(material_diffuse_color, 0.5f);
}
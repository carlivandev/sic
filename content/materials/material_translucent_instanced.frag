//? #version 410 core
//? #extension GL_ARB_bindless_texture : require
//? #extension GL_NV_gpu_shader5 : require

//! #include "../engine/materials/includes/mesh_translucent_frag_header.glsl"
//! #include "../engine/materials/includes/lighting_common.glsl"
//! #include "../engine/materials/includes/instancing_common.glsl"

flat in int frag_instanceID;

void main()
{
	vec3 material_diffuse_color = vec3(0.0f, 0.0f, 1.0f);
	//color = vec4(calculate_light_for_fragment(material_diffuse_color, position_worldspace, normal_cameraspace, eye_direction_cameraspace), 1.0f);
	color = vec4(material_diffuse_color, 0.5f);
}
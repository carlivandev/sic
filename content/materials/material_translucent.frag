#version 330 core

//! #include "includes/mesh_translucent_frag_header.glsl"
//! #include "includes/lighting_common.glsl"

// Values that stay constant for the whole mesh.
//uniform sampler2D tex_albedo;

void main()
{
	vec3 material_diffuse_color = vec3(1.0f, 0.0f, 0.0f);
	color = vec4(calculate_light_for_fragment(material_diffuse_color, position_worldspace, normal_cameraspace, eye_direction_cameraspace), 1.0f);
}
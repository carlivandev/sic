//?#version 440 core

//?#extension GL_ARB_bindless_texture : require
//?#extension GL_NV_gpu_shader5 : require

//! #include "../engine/materials/includes/mesh_vert_header.glsl"
//! #include "../engine/materials/includes/block_view.glsl"

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

flat out int frag_instanceID;

void main()
{
	// Output position of the vertex, in clip space : MVP * position
	gl_Position =  instance_data[gl_InstanceID].MVP * vec4(vertex_position_modelspace, 1);

	// Position of the vertex, in worldspace : M * position
	position_worldspace = (instance_data[gl_InstanceID].model_matrix * vec4(vertex_position_modelspace,1)).xyz;
	
	// Vector that goes from the vertex to the camera, in camera space.
	// In camera space, the camera is at the origin (0,0,0).
	vec3 vertex_position_cameraspace = ( view_matrix * instance_data[gl_InstanceID].model_matrix * vec4(vertex_position_modelspace,1)).xyz;
	eye_direction_cameraspace = vec3(0,0,0) - vertex_position_cameraspace;

	// Normal of the the vertex, in camera space
	normal_cameraspace = ( view_matrix * instance_data[gl_InstanceID].model_matrix * vec4(vertex_normal_modelspace,0)).xyz; // Only correct if ModelMatrix does not scale the model ! Use its inverse transpose if not.
	texcoord = vertex_texcoord;
	frag_instanceID = gl_InstanceID;
}
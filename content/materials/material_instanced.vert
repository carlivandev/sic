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
	uint64_t instance_data_texture;

	//how many float4 per instance
	float instance_data_texture_vec4_stride;
};

flat out int frag_instanceID;

vec4 read_vec4(sampler2D in_instance_data_tex, int in_instance_data_begin, inout int inout_current_it)
{
	vec4 vec;

	vec = texelFetch(in_instance_data_tex, ivec2(in_instance_data_begin + inout_current_it, 0), 0);
	++inout_current_it;

	return vec;
}

sampler2D read_sampler2D(sampler2D in_instance_data_tex, int in_instance_data_begin, inout int inout_current_it)
{
	uvec2 vec = uvec2(texelFetch(in_instance_data_tex, ivec2(in_instance_data_begin + inout_current_it, 0), 0).rg);
	++inout_current_it;

	return sampler2D(packUint2x32(vec));
}

mat4 read_mat4(sampler2D in_instance_data_tex, int in_instance_data_begin, inout int inout_current_it)
{
	mat4 mat;

	mat[0] = texelFetch(in_instance_data_tex, ivec2(in_instance_data_begin + inout_current_it, 0), 0);
	++inout_current_it;
	mat[1] = texelFetch(in_instance_data_tex, ivec2(in_instance_data_begin + inout_current_it, 0), 0);
	++inout_current_it;
	mat[2] = texelFetch(in_instance_data_tex, ivec2(in_instance_data_begin + inout_current_it, 0), 0);
	++inout_current_it;
	mat[3] = texelFetch(in_instance_data_tex, ivec2(in_instance_data_begin + inout_current_it, 0), 0);
	++inout_current_it;

	return mat;
}

void main()
{
	sampler2D instance_data_texture_sampler = sampler2D(instance_data_texture);

	//instancing texture only has 4 channels so its one vec4 per pixel
	int instance_data_begin = (int)instance_data_texture_vec4_stride * gl_InstanceID;
	int instance_data_it = 0;

	mat4 mvp = read_mat4(instance_data_texture_sampler, instance_data_begin, instance_data_it);
	mat4 model_matrix = read_mat4(instance_data_texture_sampler, instance_data_begin, instance_data_it);


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
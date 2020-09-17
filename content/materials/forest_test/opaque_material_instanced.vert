//?#version 440 core
//?#extension GL_ARB_bindless_texture : require
//?#extension GL_NV_gpu_shader5 : require

//! #include "../../engine/materials/includes/mesh_vert_header.glsl"
//! #include "../../engine/materials/includes/block_view.glsl"
//! #include "../../engine/materials/includes/instancing_common.glsl"

flat out int frag_instanceID;
out mat3 frag_TBN;

void main()
{
	samplerBuffer instance_data_texture_sampler = samplerBuffer(instance_data_texture);

	int instance_data_begin = get_instance_data_begin(gl_InstanceID);
	int instance_data_it = 0;
	
	mat4 mvp = read_mat4(instance_data_texture_sampler, instance_data_begin, instance_data_it);
	mat4 model_matrix = read_mat4(instance_data_texture_sampler, instance_data_begin, instance_data_it);

	// Output position of the vertex, in clip space : MVP * position
	gl_Position =  mvp * vec4(vertex_position_modelspace, 1);

	// Position of the vertex, in worldspace : M * position
	position_worldspace = (model_matrix * vec4(vertex_position_modelspace,1)).xyz;
	eye_direction_cameraspace = normalize(vec3(inverse(view_matrix)[3]) - position_worldspace);

	vec3 T = normalize(vec3(model_matrix * vec4(vertex_tangent, 0.0)));
	vec3 B = normalize(vec3(model_matrix * vec4(vertex_bitangent, 0.0)));
	vec3 N = normalize(vec3(model_matrix * vec4(vertex_normal_modelspace, 0.0)));
	frag_TBN = mat3(T, B, N);

	// Normal of the the vertex, in camera space
	normal_cameraspace = ( view_matrix * model_matrix * vec4(vertex_normal_modelspace,0)).xyz; // Only correct if ModelMatrix does not scale the model ! Use its inverse transpose if not.
	texcoord = vertex_texcoord;
	frag_instanceID = gl_InstanceID;
}
#version 330 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vertex_position_modelspace;
layout(location = 1) in vec3 vertex_normal_modelspace;
layout(location = 2) in vec2 vertex_texcoord;
layout(location = 3) in vec3 vertex_tangent;
layout(location = 4) in vec2 vertex_bitangent;

// Output data ; will be interpolated for each fragment.
out vec2 texcoord;
out vec3 position_worldspace;
out vec3 normal_cameraspace;
out vec3 eye_direction_cameraspace;
out vec3 light_direction_cameraspace;

// Values that stay constant for the whole mesh.
uniform mat4 MVP;
uniform mat4 model_matrix;
uniform vec3 light_position_worldspace;

layout(std140) uniform block_view
{
	uniform mat4 view_matrix;
};
  
void main()
{
	// Output position of the vertex, in clip space : MVP * position
	gl_Position =  MVP * vec4(vertex_position_modelspace, 1);

	// Position of the vertex, in worldspace : M * position
	position_worldspace = (model_matrix * vec4(vertex_position_modelspace,1)).xyz;
	
	// Vector that goes from the vertex to the camera, in camera space.
	// In camera space, the camera is at the origin (0,0,0).
	vec3 vertex_position_cameraspace = ( view_matrix * model_matrix * vec4(vertex_position_modelspace,1)).xyz;
	eye_direction_cameraspace = vec3(0,0,0) - vertex_position_cameraspace;

	// Vector that goes from the vertex to the light, in camera space. M is ommited because it's identity.
	vec3 light_position_cameraspace = ( view_matrix * vec4(light_position_worldspace,1)).xyz;
	light_direction_cameraspace = light_position_cameraspace + eye_direction_cameraspace;
	
	// Normal of the the vertex, in camera space
	normal_cameraspace = ( view_matrix * model_matrix * vec4(vertex_normal_modelspace,0)).xyz; // Only correct if ModelMatrix does not scale the model ! Use its inverse transpose if not.
	texcoord = vertex_texcoord;
}
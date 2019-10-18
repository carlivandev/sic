#version 330 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vertex_position_modelspace;
layout(location = 1) in vec3 vertex_normal;
layout(location = 2) in vec2 vertex_texcoord;

// Output data ; will be interpolated for each fragment.
out vec3 normal;
out vec2 texcoord;

// Values that stay constant for the whole mesh.
uniform mat4 MVP;
  
void main()
{
  // Output position of the vertex, in clip space : MVP * position
  gl_Position =  MVP * vec4(vertex_position_modelspace, 1);

  normal = vertex_normal;
  texcoord = vertex_texcoord;
}
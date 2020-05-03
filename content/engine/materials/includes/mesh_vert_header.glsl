//? #version 410 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vertex_position_modelspace;
layout(location = 1) in vec3 vertex_normal_modelspace;
layout(location = 2) in vec2 vertex_texcoord;
layout(location = 3) in vec3 vertex_tangent;
layout(location = 4) in vec3 vertex_bitangent;

// Output data ; will be interpolated for each fragment.
out vec2 texcoord;
out vec3 position_worldspace;
out vec3 normal_cameraspace;
out vec3 eye_direction_cameraspace;

// Values that stay constant for the whole mesh.
uniform mat4 MVP;
uniform mat4 model_matrix;
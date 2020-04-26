//? #version 410 core

in vec2 texcoord;
in vec3 position_worldspace;
in vec3 normal_cameraspace;
in vec3 eye_direction_cameraspace;

layout(location = 0) out vec4 color;
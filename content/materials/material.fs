#version 330 core

// Interpolated values from the vertex shaders
in vec3 normal;
// Interpolated values from the vertex shaders
in vec2 texcoord;

// Ouput data
out vec3 color;

// Values that stay constant for the whole mesh.
uniform sampler2D tex_albedo;

void main()
{
    color = texture( tex_albedo, texcoord ).rgb;
	//color = vec3(1.0f, 1.0f, 1.0f);
}
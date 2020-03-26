#version 330 core

// Interpolated values from the vertex shaders

in vec2 texcoord;
in vec3 position_worldspace;
in vec3 normal_cameraspace;
in vec3 eye_direction_cameraspace;

// Ouput data
layout(location = 0) out vec3 color;

// Values that stay constant for the whole mesh.
uniform sampler2D tex_albedo;

layout(std140) uniform block_view
{
	uniform mat4 view_matrix;
	uniform mat4 projection_matrix;
	uniform mat4 view_projection_matrix;
};

struct Light
{
	vec4 m_position_and_unused;
	vec4 m_color_and_intensity;
};

layout(std140) uniform block_lights
{
	uniform float num_lights;
	uniform Light lights[100];
};

void main()
{
	// Material properties
	vec3 material_diffuse_color = texture( tex_albedo, texcoord ).rgb;
	vec3 material_ambient_color = vec3(0.1,0.1,0.1) * material_diffuse_color;
	vec3 material_specular_color = vec3(0.3,0.3,0.3);

	// Ambient : simulates indirect lighting
	vec3 final_color = material_ambient_color;
	
	int num_lights_int = int(num_lights);
	for (int i = 0; i < num_lights_int; i++)
	{
		// Distance to the light
		float distance = length( lights[i].m_position_and_unused.xyz - position_worldspace );
	
		// Normal of the computed fragment, in camera space
		vec3 n = normalize( normal_cameraspace );
	
		vec3 light_position_cameraspace = ( view_matrix * vec4(lights[i].m_position_and_unused.xyz,1)).xyz;
		vec3 light_direction_cameraspace = light_position_cameraspace + eye_direction_cameraspace;
	
		// Direction of the light (from the fragment to the light)
		vec3 l = normalize( light_direction_cameraspace );
		// Cosine of the angle between the normal and the light direction, 
		// clamped above 0
		//  - light is at the vertical of the triangle -> 1
		//  - light is perpendicular to the triangle -> 0
		//  - light is behind the triangle -> 0
		float cosTheta = clamp( dot( n,l ), 0,1 );
	
		// Eye vector (towards the camera)
		vec3 E = normalize(eye_direction_cameraspace);
		// Direction in which the triangle reflects the light
		vec3 R = reflect(-l,n);
		// Cosine of the angle between the Eye vector and the Reflect vector,
		// clamped to 0
		//  - Looking into the reflection -> 1
		//  - Looking elsewhere -> < 1
		float cosAlpha = clamp( dot( E,R ), 0,1 );
		final_color += 
		// Diffuse : "color" of the object
		material_diffuse_color * lights[i].m_color_and_intensity.rgb * lights[i].m_color_and_intensity.a * cosTheta / (distance*distance) +
		// Specular : reflective highlight, like a mirror
		material_specular_color * lights[i].m_color_and_intensity.rgb * lights[i].m_color_and_intensity.a * pow(cosAlpha,5) / (distance*distance);
	}

	color = final_color;
}
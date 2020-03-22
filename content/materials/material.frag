#version 330 core

// Interpolated values from the vertex shaders

in vec2 texcoord;
in vec3 position_worldspace;
in vec3 normal_cameraspace;
in vec3 eye_direction_cameraspace;
in vec3 light_direction_cameraspace;

// Ouput data
layout(location = 0) out vec3 color;

// Values that stay constant for the whole mesh.
uniform sampler2D tex_albedo;
uniform vec3 light_position_worldspace;

void main()
{
    // Light emission properties
	// You probably want to put them as uniforms
	vec3 LightColor = vec3(0,1,1);
	float LightPower = 80.0f;
	
	// Material properties
	vec3 material_diffuse_color = texture( tex_albedo, texcoord ).rgb;
	vec3 material_ambient_color = vec3(0.1,0.1,0.1) * material_diffuse_color;
	vec3 material_specular_color = vec3(0.3,0.3,0.3);

	// Distance to the light
	float distance = length( light_position_worldspace - position_worldspace );

	// Normal of the computed fragment, in camera space
	vec3 n = normalize( normal_cameraspace );
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
	
	color = 
		// Ambient : simulates indirect lighting
		material_ambient_color +
		// Diffuse : "color" of the object
		material_diffuse_color * LightColor * LightPower * cosTheta / (distance*distance) +
		// Specular : reflective highlight, like a mirror
		material_specular_color * LightColor * LightPower * pow(cosAlpha,5) / (distance*distance);
}
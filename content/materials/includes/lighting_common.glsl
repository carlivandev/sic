//? #version 410 core

//! #include "block_lights.glsl"
//! #include "block_view.glsl"

vec3 calculate_light_for_fragment(vec3 in_fragment_diffuse_color, vec3 in_fragment_world_position, vec3 in_fragment_normal_cameraspace, vec3 in_fragment_to_view_direction)
{
	vec3 material_ambient_color = vec3(0.1,0.1,0.1) * in_fragment_diffuse_color;
	vec3 material_specular_color = vec3(0.3,0.3,0.3);

	// Ambient : simulates indirect lighting
	vec3 final_color = material_ambient_color;
	
	int num_lights_int = int(num_lights);
	for (int i = 0; i < num_lights_int; i++)
	{
		// Distance to the light
		float distance = length( lights[i].m_position_and_unused.xyz - in_fragment_world_position );
	
		// Normal of the computed fragment, in camera space
		vec3 n = normalize( in_fragment_normal_cameraspace );
	
		vec3 light_position_cameraspace = ( view_matrix * vec4(lights[i].m_position_and_unused.xyz,1)).xyz;
		vec3 light_direction_cameraspace = light_position_cameraspace + in_fragment_to_view_direction;
	
		// Direction of the light (from the fragment to the light)
		vec3 l = normalize( light_direction_cameraspace );
		// Cosine of the angle between the normal and the light direction, 
		// clamped above 0
		//  - light is at the vertical of the triangle -> 1
		//  - light is perpendicular to the triangle -> 0
		//  - light is behind the triangle -> 0
		float cosTheta = clamp( dot( n,l ), 0,1 );
	
		// Eye vector (towards the camera)
		vec3 E = normalize(in_fragment_to_view_direction);
		// Direction in which the triangle reflects the light
		vec3 R = reflect(-l,n);
		// Cosine of the angle between the Eye vector and the Reflect vector,
		// clamped to 0
		//  - Looking into the reflection -> 1
		//  - Looking elsewhere -> < 1
		float cosAlpha = clamp( dot( E,R ), 0,1 );
		final_color += 
		// Diffuse : "color" of the object
		in_fragment_diffuse_color * lights[i].m_color_and_intensity.rgb * lights[i].m_color_and_intensity.a * cosTheta / (distance*distance) +
		// Specular : reflective highlight, like a mirror
		material_specular_color * lights[i].m_color_and_intensity.rgb * lights[i].m_color_and_intensity.a * pow(cosAlpha,5) / (distance*distance);
	}

	return final_color;
}
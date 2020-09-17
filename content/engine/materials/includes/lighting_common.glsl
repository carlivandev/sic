//? #version 410 core

//! #include "block_lights.glsl"
//! #include "block_view.glsl"

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}  

vec3 calculate_light_for_fragment_pbr(vec3 in_albedo, vec3 in_normal, float in_metallic, float in_roughness, float in_ao, vec3 in_world_position, vec3 in_fragment_to_view_direction)
{
	vec3 N = in_normal;
    vec3 V = in_fragment_to_view_direction;

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, in_albedo, in_metallic);
	           
    // reflectance equation
	vec3 Lo = vec3(0.0);

    int num_lights_int = int(num_lights);
	for (int i = 0; i < num_lights_int; i++)
    {
		vec3 light_pos = lights[i].m_position_and_unused.xyz;
		float light_intensity = lights[i].m_color_and_intensity.a;
		vec3 light_color = lights[i].m_color_and_intensity.rgb * light_intensity;

        // calculate per-light radiance
        vec3 L = normalize(light_pos - in_world_position);
        vec3 H = normalize(V + L);
        float distance = length(light_pos - in_world_position);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance     = light_color * attenuation;        
        
        // cook-torrance brdf
        float NDF = DistributionGGX(N, H, in_roughness);        
        float G   = GeometrySmith(N, V, L, in_roughness);      
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);       
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - in_metallic;	  
        
        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
        vec3 specular     = numerator / max(denominator, 0.001);  
            
        // add to outgoing radiance Lo
        float NdotL = max(dot(N, L), 0.0);                
		Lo += (kD * in_albedo / PI + specular) + radiance * NdotL;
		//Lo +=  radiance * NdotL;
    }   
  
    vec3 ambient = vec3(0.02) * in_albedo * in_ao;
    vec3 color = ambient + Lo;
	
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));  
   
	return color;
}

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
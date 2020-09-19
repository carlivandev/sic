//? #version 410 core

//! #include "block_lights.glsl"
//! #include "block_view.glsl"

const float PI = 3.14159265359;

const float Epsilon = 0.00001;
// Constant normal incidence Fresnel factor for all dielectrics.
const vec3 Fdielectric = vec3(0.04);

// GGX/Towbridge-Reitz normal distribution function.
// Uses Disney's reparametrization of alpha = roughness^2.
float ndfGGX(float cosLh, float roughness)
{
	float alpha   = roughness * roughness;
	float alphaSq = alpha * alpha;

	float denom = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
	return alphaSq / (PI * denom * denom);
}

// Single term for separable Schlick-GGX below.
float gaSchlickG1(float cosTheta, float k)
{
	return cosTheta / (cosTheta * (1.0 - k) + k);
}

// Schlick-GGX approximation of geometric attenuation function using Smith's method.
float gaSchlickGGX(float cosLi, float cosLo, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0; // Epic suggests using this roughness remapping for analytic lights.
	return gaSchlickG1(cosLi, k) * gaSchlickG1(cosLo, k);
}

// Shlick's approximation of the Fresnel factor.
vec3 fresnelSchlick(vec3 F0, float cosTheta)
{
	return F0 + (vec3(1.0) - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 calculate_light_for_fragment_pbr(vec3 in_albedo, vec3 in_normal, float in_metalness, float in_roughness, float in_ao, vec3 in_world_position, vec3 in_fragment_to_view_direction)
{
	// Outgoing light direction (vector from world-space fragment position to the "eye").
	vec3 Lo = in_fragment_to_view_direction;

	// Get current fragment's normal and transform to world space.
	vec3 N = in_normal;
	
	// Angle between surface normal and outgoing light direction.
	float cosLo = max(0.0, dot(N, Lo));
		
	// Specular reflection vector.
	vec3 Lr = 2.0 * cosLo * N - Lo;

	// Fresnel reflectance at normal incidence (for metals use albedo color).
	vec3 F0 = mix(Fdielectric, in_albedo, in_metalness);

	// Direct lighting calculation for analytical lights.
	vec3 directLighting = vec3(0);

	int num_lights_int = int(num_lights);
	for (int i = 0; i < num_lights_int; ++i)
	{
		vec3 light_pos = lights[i].m_position_and_unused.xyz;
		float light_intensity = lights[i].m_color_and_intensity.a;
		vec3 light_color = lights[i].m_color_and_intensity.rgb * light_intensity;

		vec3 light_dir = normalize(in_world_position - light_pos);

		vec3 Li = -light_dir;
		vec3 Lradiance = light_color;


		// Half-vector between Li and Lo.
		vec3 Lh = normalize(Li + Lo);

		// Calculate angles between surface normal and various light vectors.
		float cosLi = max(0.0, dot(N, Li));
		float cosLh = max(0.0, dot(N, Lh));

		// Calculate Fresnel term for direct lighting. 
		vec3 F  = fresnelSchlick(F0, max(0.0, dot(Lh, Lo)));
		// Calculate normal distribution for specular BRDF.
		float D = ndfGGX(cosLh, in_roughness);
		// Calculate geometric attenuation for specular BRDF.
		float G = gaSchlickGGX(cosLi, cosLo, in_roughness);

		// Diffuse scattering happens due to light being refracted multiple times by a dielectric medium.
		// Metals on the other hand either reflect or absorb energy, so diffuse contribution is always zero.
		// To be energy conserving we must scale diffuse BRDF contribution based on Fresnel factor & metalness.
		vec3 kd = mix(vec3(1.0) - F, vec3(0.0), in_metalness);

		// Lambert diffuse BRDF.
		// We don't scale by 1/PI for lighting & material units to be more convenient.
		// See: https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
		vec3 diffuseBRDF = kd * in_albedo;

		// Cook-Torrance specular microfacet BRDF.
		vec3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * cosLo);

		// Total contribution for this light.
		directLighting += (diffuseBRDF + specularBRDF) * Lradiance * cosLi;
	}

	// Ambient lighting (IBL).
	//vec3 ambientLighting;
	//{
	//	// Sample diffuse irradiance at normal direction.
	//	vec3 irradiance = texture(irradianceTexture, N).rgb;
	//
	//	// Calculate Fresnel term for ambient lighting.
	//	// Since we use pre-filtered cubemap(s) and irradiance is coming from many directions
	//	// use cosLo instead of angle with light's half-vector (cosLh above).
	//	// See: https://seblagarde.wordpress.com/2011/08/17/hello-world/
	//	vec3 F = fresnelSchlick(F0, cosLo);
	//
	//	// Get diffuse contribution factor (as with direct lighting).
	//	vec3 kd = mix(vec3(1.0) - F, vec3(0.0), metalness);
	//
	//	// Irradiance map contains exitant radiance assuming Lambertian BRDF, no need to scale by 1/PI here either.
	//	vec3 diffuseIBL = kd * albedo * irradiance;
	//
	//	// Sample pre-filtered specular reflection environment at correct mipmap level.
	//	int specularTextureLevels = textureQueryLevels(specularTexture);
	//	vec3 specularIrradiance = textureLod(specularTexture, Lr, roughness * specularTextureLevels).rgb;
	//
	//	// Split-sum approximation factors for Cook-Torrance specular BRDF.
	//	vec2 specularBRDF = texture(specularBRDF_LUT, vec2(cosLo, roughness)).rg;
	//
	//	// Total specular IBL contribution.
	//	vec3 specularIBL = (F0 * specularBRDF.x + specularBRDF.y) * specularIrradiance;
	//
	//	// Total ambient lighting contribution.
	//	ambientLighting = diffuseIBL + specularIBL;
	//}

	// Final fragment color.
	vec3 final_color = directLighting;
	//final_color += ambientLighting;

	//fake ambiant light
	vec3 material_ambient_color = vec3(0.4, 0.5, 0.7) * in_albedo;
	final_color += material_ambient_color;

	return final_color;
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
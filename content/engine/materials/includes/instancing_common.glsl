//? #version 410 core
//?#extension GL_ARB_bindless_texture : require
//?#extension GL_NV_gpu_shader5 : require

layout(std140) uniform block_instancing
{
	//how many vec4 per instance
	vec4 instance_data_texture_vec4_stride;
	uint64_t instance_data_texture;
};

int get_instance_data_begin(int in_instance_id)
{
	//instancing texture has 4 channels so its one vec4 per pixel
	return (int)instance_data_texture_vec4_stride.x * in_instance_id;
}

vec4 read_vec4(sampler2D in_instance_data_tex, int in_instance_data_begin, inout int inout_current_it)
{
	vec4 vec;

	vec = texelFetch(in_instance_data_tex, ivec2(in_instance_data_begin + inout_current_it, 0), 0);
	++inout_current_it;

	return vec;
}

sampler2D read_sampler2D(sampler2D in_instance_data_tex, int in_instance_data_begin, inout int inout_current_it)
{
	vec2 vec = texelFetch(in_instance_data_tex, ivec2(in_instance_data_begin + inout_current_it, 0), 0).xy;
	++inout_current_it;
	
	return sampler2D(uvec2(floatBitsToUint(vec.x), floatBitsToUint(vec.y)));
}

mat4 read_mat4(sampler2D in_instance_data_tex, int in_instance_data_begin, inout int inout_current_it)
{
	mat4 mat;

	mat[0] = texelFetch(in_instance_data_tex, ivec2(in_instance_data_begin + inout_current_it, 0), 0);
	++inout_current_it;
	mat[1] = texelFetch(in_instance_data_tex, ivec2(in_instance_data_begin + inout_current_it, 0), 0);
	++inout_current_it;
	mat[2] = texelFetch(in_instance_data_tex, ivec2(in_instance_data_begin + inout_current_it, 0), 0);
	++inout_current_it;
	mat[3] = texelFetch(in_instance_data_tex, ivec2(in_instance_data_begin + inout_current_it, 0), 0);
	++inout_current_it;

	return mat;
}
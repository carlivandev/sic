#pragma once
#include "sic/asset_texture.h"
#include "sic/defines.h"
#include "sic/system_asset.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

void sic::Asset_texture_base::load(const State_assetsystem& in_assetsystem_state, Deserialize_stream& in_stream)
{
	in_assetsystem_state;

	in_stream.read(m_width);
	in_stream.read(m_height);
	in_stream.read(m_format);
}

void sic::Asset_texture_base::save(const State_assetsystem& in_assetsystem_state, Serialize_stream& out_stream) const
{
	in_assetsystem_state;

	out_stream.write(m_width);
	out_stream.write(m_height);
	out_stream.write(m_format);
}

void sic::Asset_texture::load(const State_assetsystem& in_assetsystem_state, Deserialize_stream& in_stream)
{
	Asset_texture_base::load(in_assetsystem_state, in_stream);

	unsigned char* texture_data = nullptr;
	in_stream.read_raw(texture_data, m_texture_data_bytesize);

	if (texture_data != nullptr)
		m_texture_data = std::unique_ptr<unsigned char>(texture_data);
}

void sic::Asset_texture::save(const State_assetsystem& in_assetsystem_state, Serialize_stream& out_stream) const
{
	Asset_texture_base::save(in_assetsystem_state, out_stream);
	
	out_stream.write_raw(m_texture_data.get(), m_texture_data_bytesize);
}

bool sic::Asset_texture::import(std::string&& in_data, bool in_free_texture_data_after_setup, bool in_flip_vertically)
{
	sic::i32 width, height, channel_count;

	stbi_set_flip_vertically_on_load(in_flip_vertically);
	stbi_uc* texture_data = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(in_data.c_str()), static_cast<int>(in_data.size()), &width, &height, &channel_count, 0);

	if (!texture_data)
		return false;
	
	m_texture_data = std::unique_ptr<unsigned char>(texture_data);

	m_width = width;
	m_height = height;
	m_format = static_cast<Texture_format>(channel_count);
	m_texture_data_bytesize = channel_count * m_width * m_height;
	m_free_texture_data_after_setup = in_free_texture_data_after_setup;

	return true;
}

void sic::Asset_render_target::initialize(Texture_format in_format, const glm::ivec2& in_dimensions, bool in_depth_test)
{
	m_format = in_format;

	m_width = in_dimensions.x;
	m_height = in_dimensions.y;

	m_depth_test = in_depth_test;
}

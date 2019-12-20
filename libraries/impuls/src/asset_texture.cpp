#pragma once
#include "impuls/asset_texture.h"
#include "impuls/defines.h"
#include "impuls/system_asset.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

void sic::Asset_texture::load(const State_assetsystem& in_assetsystem_state, Deserialize_stream& in_stream)
{
	in_assetsystem_state;

	in_stream.read(m_width);
	in_stream.read(m_height);
	in_stream.read(m_channel_count);
	in_stream.read(m_format);
	in_stream.read(m_texture_data);
}

void sic::Asset_texture::save(const State_assetsystem& in_assetsystem_state, Serialize_stream& out_stream) const
{
	in_assetsystem_state;

	out_stream.write(m_width);
	out_stream.write(m_height);
	out_stream.write(m_channel_count);
	out_stream.write(m_format);
	out_stream.write(m_texture_data);
}

bool sic::Asset_texture::import(std::string&& in_data, Texture_format in_format, bool in_free_texture_data_after_setup)
{
	sic::i32 width, height, channel_count;

	stbi_uc* texture_data = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(in_data.c_str()), static_cast<int>(in_data.size()), &width, &height, &channel_count, static_cast<sic::i8>(in_format));

	if (!texture_data)
		return false;
	
	m_texture_data = std::unique_ptr<unsigned char>(texture_data);

	if (channel_count != static_cast<sic::i8>(in_format))
	{
		//printf("texture load channel size mismatch, expected: %i, source: \"%s\"\n", channel_count, in_filepath);
		return false;
	}

	m_width = width;
	m_height = height;
	m_channel_count = channel_count;
	m_format = in_format;
	m_free_texture_data_after_setup = in_free_texture_data_after_setup;

	return true;
}

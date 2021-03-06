#pragma once
#include "sic/asset_texture.h"
#include "sic/system_asset.h"

#include "sic/core/defines.h"

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

	void* texture_data = nullptr;
	in_stream.read_raw(texture_data, m_texture_data_bytesize);

	if (texture_data != nullptr)
		m_texture_data = std::unique_ptr<unsigned char>((unsigned char*)texture_data);
}

void sic::Asset_texture::save(const State_assetsystem& in_assetsystem_state, Serialize_stream& out_stream) const
{
	Asset_texture_base::save(in_assetsystem_state, out_stream);
	
	out_stream.write_raw(m_texture_data.get(), m_texture_data_bytesize);
}

void sic::Asset_render_target::initialize(Texture_format in_format, const glm::ivec2& in_dimensions, bool in_depth_test)
{
	m_format = in_format;

	m_width = in_dimensions.x;
	m_height = in_dimensions.y;

	m_depth_test = in_depth_test;
}

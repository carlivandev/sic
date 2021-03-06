#pragma once
#include "sic/asset_font.h"

#include "sic/core/defines.h"

sic::Log sic::g_log_font("Font");
sic::Log sic::g_log_font_verbose("Font", false);

void sic::Asset_font::load(const State_assetsystem& in_assetsystem_state, Deserialize_stream& in_stream)
{
	Asset_texture_base::load(in_assetsystem_state, in_stream);

	void* atlas_data = nullptr;
	in_stream.read_raw(atlas_data, m_atlas_data_bytesize);

	if (atlas_data != nullptr)
		m_atlas_data = std::unique_ptr<unsigned char>((unsigned char*)atlas_data);

	void* glyphs_data = nullptr;
	ui32 glyphs_data_bytesize = 0;
	in_stream.read_raw(glyphs_data, glyphs_data_bytesize);
	m_glyphs.resize(glyphs_data_bytesize * sizeof(Glyph));
	memcpy(m_glyphs.data(), glyphs_data, glyphs_data_bytesize);
}

void sic::Asset_font::save(const State_assetsystem& in_assetsystem_state, Serialize_stream& out_stream) const
{
	Asset_texture_base::save(in_assetsystem_state, out_stream);
	out_stream.write_raw(m_atlas_data.get(), m_atlas_data_bytesize);
	out_stream.write_raw(m_glyphs.data(), static_cast<ui32>(m_glyphs.size() * sizeof(Glyph)));
}

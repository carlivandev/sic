#pragma once
#include "sic/defines.h"
#include "sic/asset.h"
#include "sic/opengl_texture.h"

#include <optional>

namespace sic
{
	enum struct Texture_format
	{
		invalid = 0,	/*STBI_default*/
		gray = 1,		/*STBI_grey*/
		gray_a = 2,		/*STBI_grey_alpha*/
		rgb = 3,		/*STBI_rgb*/
		rgb_a = 4		/*STBI_rgb_alpha*/
	};

	struct Asset_texture : Asset
	{
		bool has_post_load() override final { return true; }
		bool has_pre_unload() override final { return true; }

		void load(const State_assetsystem& in_assetsystem_state, Deserialize_stream& in_stream) override;
		void save(const State_assetsystem& in_assetsystem_state, Serialize_stream& out_stream) const override;
		bool import(std::string&& in_data, Texture_format in_format, bool in_free_texture_data_after_setup);

		sic::i32 m_width = 0;
		sic::i32 m_height = 0;
		sic::i32 m_channel_count = 0;

		Texture_format m_format = Texture_format::invalid;

		std::unique_ptr<unsigned char> m_texture_data;
		ui32 m_texture_data_bytesize = 0;

		std::optional<OpenGl_texture> m_texture;

		bool m_free_texture_data_after_setup = false;
	};
}
#pragma once
#include "impuls/defines.h"
#include "impuls/gl_includes.h"
#include "impuls/asset.h"

#include <memory>

namespace impuls
{
	enum class e_texture_format
	{
		invalid = 0,	/*STBI_default*/
		gray = 1,		/*STBI_grey*/
		gray_a = 2,		/*STBI_grey_alpha*/
		rgb = 3,		/*STBI_rgb*/
		rgb_a = 4		/*STBI_rgb_alpha*/
	};

	struct asset_texture : i_asset
	{
		bool has_post_load() override final { return true; }
		bool has_pre_unload() override final { return true; }

		void load(const state_assetsystem& in_assetsystem_state, deserialize_stream& in_stream) override;
		void save(const state_assetsystem& in_assetsystem_state, serialize_stream& out_stream) const override;
		bool import(std::string&& in_data, e_texture_format in_format, bool in_free_texture_data_after_setup);

		impuls::i32 m_width = 0;
		impuls::i32 m_height = 0;
		impuls::i32 m_channel_count = 0;

		e_texture_format m_format = e_texture_format::invalid;

		unsigned char* m_texture_data = nullptr;
		GLuint m_render_id = 0;
		bool m_free_texture_data_after_setup = false;
	};
}
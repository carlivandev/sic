#pragma once
#include "sic/defines.h"
#include "sic/asset.h"
#include "sic/opengl_texture.h"
#include "sic/opengl_render_target.h"

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

	struct Asset_texture_base : Asset
	{
		bool has_post_load() override final { return true; }
		bool has_pre_unload() override final { return true; }

		virtual void load(const State_assetsystem& in_assetsystem_state, Deserialize_stream& in_stream) override;
		virtual void save(const State_assetsystem& in_assetsystem_state, Serialize_stream& out_stream) const override;

		sic::i32 m_width = 0;
		sic::i32 m_height = 0;
		Texture_format m_format = Texture_format::invalid;

		GLuint64 m_bindless_handle = 0;
	};

	struct Asset_texture : Asset_texture_base
	{
		void load(const State_assetsystem& in_assetsystem_state, Deserialize_stream& in_stream) override;
		void save(const State_assetsystem& in_assetsystem_state, Serialize_stream& out_stream) const override;
		bool import(std::string&& in_data, bool in_free_texture_data_after_setup, bool in_flip_vertically);

		std::unique_ptr<unsigned char> m_texture_data;
		ui32 m_texture_data_bytesize = 0;

		std::optional<OpenGl_texture> m_texture;

		bool m_free_texture_data_after_setup = false;
	};

	struct Asset_render_target : Asset_texture_base
	{
		void initialize(Texture_format in_format, const glm::ivec2& in_dimensions, bool in_depth_test);

		std::optional<OpenGl_render_target> m_render_target;
		bool m_depth_test = false;
	};
}
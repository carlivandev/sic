#pragma once
#include "sic/asset.h"
#include "sic/asset_texture.h"

#include <string>

namespace sic
{
	extern Log g_log_font;
	extern Log g_log_font_verbose;

	struct Asset_font : Asset_texture_base
	{
		/// Type of atlas image contents
		enum class Image_type
		{
			/// Rendered glyphs without anti-aliasing (two colors only)
			HARD_MASK,
			/// Rendered glyphs with anti-aliasing
			SOFT_MASK,
			/// Signed (true) distance field
			SDF,
			/// Signed pseudo-distance field
			PSDF,
			/// Multi-channel signed distance field
			MSDF,
			/// Multi-channel & true signed distance field
			MTSDF
		};

		/// Atlas image encoding
		enum class Image_format
		{
			UNSPECIFIED,
			PNG,
			BMP,
			TIFF,
			TEXT,
			TEXT_FLOAT,
			BINARY,
			BINARY_FLOAT,
			BINARY_FLOAT_BE
		};

		struct Import_configuration
		{
			Asset_font::Image_type m_image_type = Asset_font::Image_type::MSDF;
			Asset_font::Image_format m_image_format = Asset_font::Image_format::UNSPECIFIED;
			int m_width = 0;
			int m_height = 0;
			double m_em_size = 32.0;
			double m_px_range = 2.0;
			double m_angle_threshold = 3.0;
			double m_miter_limit = 1.0;
			unsigned long long m_coloring_seed = 0;
		};

		struct Glyph
		{
			glm::vec2 m_atlas_pixel_position;
			glm::vec2 m_atlas_pixel_size;
			glm::vec2 m_offset_to_glyph;
			std::vector<float> m_kerning_values;
			float m_pixel_advance;
		};

		void load(const State_assetsystem& in_assetsystem_state, Deserialize_stream& in_stream) override;
		void save(const State_assetsystem& in_assetsystem_state, Serialize_stream& out_stream) const override;

		std::vector<Glyph> m_glyphs;
		float m_max_glyph_height = 0.0f;
		float m_baseline = 0.0f;
		float m_em_size = 32.0f;

		std::unique_ptr<unsigned char> m_atlas_data;
		ui32 m_atlas_data_bytesize = 0;

		std::optional<OpenGl_texture> m_texture;
	};
}
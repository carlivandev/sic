#pragma once
#include "sic/asset.h"
#include "sic/asset_texture.h"

#include <string>

// #define SDF_ERROR_ESTIMATE_PRECISION 19
// #define GLYPH_FILL_RULE msdfgen::FILL_NONZERO


namespace sic
{
	struct Asset_font : Asset_texture_base
	{
		/// Type of atlas image contents
		enum class ImageType {
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
		enum class ImageFormat {
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

		struct Import_configuration {
			Asset_font::ImageType imageType = Asset_font::ImageType::MSDF;
			Asset_font::ImageFormat imageFormat = Asset_font::ImageFormat::UNSPECIFIED;
			int width = 0;
			int height = 0;
			double emSize = 32.0;
			double pxRange = 2.0;
			double angleThreshold = 3.0;
			double miterLimit = 1.0;
			unsigned long long coloringSeed = 0;
			int threadCount = 1;
		};

		struct Glyph
		{
			glm::vec2 m_atlas_pixel_position;
			glm::vec2 m_atlas_pixel_size;
			glm::vec2 m_offset_to_glyph;
			float m_pixel_advance;
		};

		void load(const State_assetsystem& in_assetsystem_state, Deserialize_stream& in_stream) override;
		void save(const State_assetsystem& in_assetsystem_state, Serialize_stream& out_stream) const override;
		bool import(std::string&& in_font_path, const Import_configuration& in_configuration);

		std::vector<Glyph> m_glyphs;
		float m_max_glyph_height = 0;

		std::unique_ptr<unsigned char> m_atlas_data;
		ui32 m_atlas_data_bytesize = 0;
		std::optional<OpenGl_texture> m_texture;
	};
}
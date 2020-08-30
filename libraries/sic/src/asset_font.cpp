#pragma once
#include "sic/asset_font.h"
#include "sic/defines.h"

#include "msdf-atlas-gen/msdf-atlas-gen.h"

#include <filesystem>

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
	out_stream.write_raw(m_glyphs.data(), m_glyphs.size() * sizeof(Glyph));
}

namespace sic_font_private
{
	static void load_glyphs(msdfgen::FontHandle* in_font, const msdf_atlas::Charset& in_charset, std::vector<msdf_atlas::GlyphGeometry>& out_glyphs)
	{
		
	}
}

bool sic::Asset_font::import(const std::string& in_font_path, Import_configuration in_configuration)
{
	msdf_atlas::GeneratorAttributes generator_attributes;
	generator_attributes.overlapSupport = true;
	generator_attributes.scanlinePass = true;
	generator_attributes.errorCorrectionThreshold = MSDFGEN_DEFAULT_ERROR_CORRECTION_THRESHOLD;

	double min_em_size = 0;
	enum
	{
		/// Range specified in EMs
		RANGE_EM,
		/// Range specified in output pixels
		RANGE_PIXEL,
	} range_mode = RANGE_PIXEL;
	double range_value = 0;
	msdf_atlas::TightAtlasPacker::DimensionsConstraint atlas_size_constraint = msdf_atlas::TightAtlasPacker::DimensionsConstraint::MULTIPLE_OF_FOUR_SQUARE;

	// Fix up configuration based on related values
	if (!(in_configuration.m_image_type == Image_type::PSDF || in_configuration.m_image_type == Image_type::MSDF || in_configuration.m_image_type == Image_type::MTSDF))
		in_configuration.m_miter_limit = 0;
	if (in_configuration.m_em_size > min_em_size)
		min_em_size = in_configuration.m_em_size;

	if (!(in_configuration.m_image_type == Image_type::SDF || in_configuration.m_image_type == Image_type::PSDF || in_configuration.m_image_type == Image_type::MSDF || in_configuration.m_image_type == Image_type::MTSDF))
	{
		range_mode = RANGE_PIXEL;
		range_value = (double)(in_configuration.m_image_type == Image_type::SOFT_MASK);
	}
	else if (range_value <= 0)
	{
		range_mode = RANGE_PIXEL;
		range_value = 2.0;
	}

	// Load font
	struct Font_holder
	{
		msdfgen::FreetypeHandle* m_ft;
		msdfgen::FontHandle* m_font;

		explicit Font_holder(const char* in_font_filename) : m_ft(nullptr), m_font(nullptr)
		{
			if ((m_ft = msdfgen::initializeFreetype()))
				m_font = msdfgen::loadFont(m_ft, in_font_filename);
		}
		~Font_holder()
		{
			if (m_ft)
			{
				if (m_font)
					msdfgen::destroyFont(m_font);

				msdfgen::deinitializeFreetype(m_ft);
			}
		}
		operator msdfgen::FontHandle* () const {
			return m_font;
		}
	} font(std::filesystem::absolute(std::filesystem::path(in_font_path.c_str())).string().c_str());

	if (!font)
	{
		SIC_LOG_E(g_log_font, "Failed to load font with path: \"{0}\"", in_font_path.c_str());
		return false;
	}

	msdfgen::FontMetrics font_metrics = { };
	msdfgen::getFontMetrics(font_metrics, font);
	if (font_metrics.emSize <= 0)
		font_metrics.emSize = 32.0;

	// Load character set
	msdf_atlas::Charset charset = msdf_atlas::Charset::ASCII;

	// Load glyphs
	std::vector<msdf_atlas::GlyphGeometry> glyphs;

	{
		glyphs.reserve(charset.size());
		for (msdf_atlas::unicode_t cp : charset)
		{
			msdf_atlas::GlyphGeometry glyph;

			if (glyph.load(font, cp))
				glyphs.push_back((msdf_atlas::GlyphGeometry&&)glyph);
			else
				SIC_LOG_E(g_log_font, "Glyph for codepoint {0} missing. Font path: \"{1}\"", cp, in_font_path.c_str());
		}
	}

	sic_font_private::load_glyphs(font, charset, glyphs);

	SIC_LOG_V(g_log_font, "Successfully loaded {0}/{1} character geometry. Font path: \"{2}\"", glyphs.size(), charset.size(), in_font_path.c_str());

	// Determine final atlas dimensions, scale and range, pack glyphs
	{
		double unit_range = 0, px_range = 0;

		switch (range_mode)
		{
		case RANGE_EM:
			unit_range = range_value * font_metrics.emSize;
			break;
		case RANGE_PIXEL:
			px_range = range_value;
			break;
		}

		const bool uses_fixed_scale = in_configuration.m_em_size > 0;

		msdf_atlas::TightAtlasPacker atlas_packer;

		atlas_packer.setDimensionsConstraint(atlas_size_constraint);
		atlas_packer.setPadding(in_configuration.m_image_type == Image_type::MSDF || in_configuration.m_image_type == Image_type::MTSDF ? 0 : -1);
		// TODO: In this case (if padding is -1), the border pixels of each glyph are black, but still computed. For floating-point output, this may play a role.
		if (uses_fixed_scale)
			atlas_packer.setScale(in_configuration.m_em_size / font_metrics.emSize);
		else
			atlas_packer.setMinimumScale(min_em_size / font_metrics.emSize);
		atlas_packer.setPixelRange(px_range);
		atlas_packer.setUnitRange(unit_range);
		atlas_packer.setMiterLimit(in_configuration.m_miter_limit);

		if (int remaining = atlas_packer.pack(glyphs.data(), glyphs.size()))
		{
			if (remaining < 0)
			{
				SIC_LOG_E(g_log_font, "Failed to pack glyphs into atlas. Font path: \"{0}\"", in_font_path.c_str());
				return false;
			}
			else
			{
				SIC_LOG_E(g_log_font, "Could not fit {0} out of {1} glyphs into the atlas. Font path: \"{2}\"", remaining, (int)glyphs.size(), in_font_path.c_str());
				return false;
			}
		}

		atlas_packer.getDimensions(in_configuration.m_width, in_configuration.m_height);
		if (!(in_configuration.m_width > 0 && in_configuration.m_height > 0))
		{
			SIC_LOG_E(g_log_font, "Unable to determine atlas size. Font path: \"{0}\"", in_font_path.c_str());
			return false;
		}

		in_configuration.m_em_size = atlas_packer.getScale() * font_metrics.emSize;
		in_configuration.m_px_range = atlas_packer.getPixelRange();

		SIC_LOG_V(g_log_font_verbose, "Successfully generated font atlas. Atlas dimensions: {0}x{1}. Font path: \"{2}\"", in_configuration.m_width, in_configuration.m_height, in_font_path.c_str());
	}

	// Generate atlas bitmap
	// Edge coloring
	if (in_configuration.m_image_type == Image_type::MSDF || in_configuration.m_image_type == Image_type::MTSDF)
	{
		unsigned long long glyph_seed = in_configuration.m_coloring_seed;
		for (msdf_atlas::GlyphGeometry& glyph : glyphs)
		{
			glyph_seed *= 6364136223846793005ull;
			glyph.edgeColoring(in_configuration.m_angle_threshold, glyph_seed);
		}
	}

	msdf_atlas::ImmediateAtlasGenerator<float, 3, msdf_atlas::msdfGenerator, msdf_atlas::BitmapAtlasStorage<float, 3> > generator(in_configuration.m_width, in_configuration.m_height);
	generator.setAttributes(generator_attributes);
	generator.setThreadCount(1);
	generator.generate(glyphs.data(), glyphs.size());
	msdfgen::BitmapConstRef<float, 3> bitmap = (msdfgen::BitmapConstRef<float, 3>) generator.atlasStorage();

	m_atlas_data_bytesize = 3 * bitmap.width * bitmap.height * sizeof(float);
	m_atlas_data = std::unique_ptr<unsigned char>(new unsigned char[m_atlas_data_bytesize]);
	memcpy(m_atlas_data.get(), bitmap.pixels, m_atlas_data_bytesize);

	m_width = bitmap.width;
	m_height = bitmap.height;
	m_format = Texture_format::rgb;

	for (const msdf_atlas::GlyphGeometry& glyph : glyphs)
	{
		if (m_glyphs.size() <= glyph.getCodepoint())
			m_glyphs.resize(glyph.getCodepoint() + 1);

		auto& new_glyph = m_glyphs[glyph.getCodepoint()];

		msdf_atlas::GlyphBox gb = glyph;
		new_glyph.m_atlas_pixel_position.x = gb.rect.x;
		new_glyph.m_atlas_pixel_position.y = gb.rect.y;

		new_glyph.m_offset_to_glyph = glm::vec2{ - glyph.getBoxTranslate().x, -gb.rect.h + glyph.getBoxTranslate().y };

		new_glyph.m_atlas_pixel_size.x = gb.rect.w;
		new_glyph.m_atlas_pixel_size.y = gb.rect.h;
		
		new_glyph.m_pixel_advance = glyph.getAdvance() * (in_configuration.m_em_size / font_metrics.emSize);
	}

	for (auto& glyph : m_glyphs)
		glyph.m_kerning_values.resize(m_glyphs.size());
	
	for (const msdf_atlas::GlyphGeometry& glyph_left : glyphs)
	{
		for (const msdf_atlas::GlyphGeometry& glyph_right : glyphs)
		{
			if (&glyph_left == &glyph_right)
				continue;

			auto& kerning_to_modify = m_glyphs[glyph_right.getCodepoint()].m_kerning_values[glyph_left.getCodepoint()];

			double kerning;
			if (msdfgen::getKerning(kerning, font.m_font, glyph_left.getCodepoint(), glyph_right.getCodepoint()))
				kerning_to_modify = kerning;
			else
				kerning_to_modify = 0.0f;
		}
	}

	m_max_glyph_height = font_metrics.ascenderY - font_metrics.descenderY;
	m_baseline = font_metrics.lineHeight;
	m_em_size = in_configuration.m_em_size;

	//msdf_atlas::saveImage(bitmap, msdf_atlas::ImageFormat::PNG, "test.png");

	return true;
}

#pragma once
#include "sic/asset_font.h"
#include "sic/defines.h"

#include "msdf-atlas-gen/msdf-atlas-gen.h"

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

static void loadGlyphs(std::vector<msdf_atlas::GlyphGeometry>& glyphs, msdfgen::FontHandle* font, const msdf_atlas::Charset& charset) {
	glyphs.clear();
	glyphs.reserve(charset.size());
	for (msdf_atlas::unicode_t cp : charset) {
		msdf_atlas::GlyphGeometry glyph;
		if (glyph.load(font, cp))
			glyphs.push_back((msdf_atlas::GlyphGeometry&&)glyph);
		else
			printf("Glyph for codepoint 0x%X missing\n", cp);
	}
}

bool sic::Asset_font::import(std::string&& in_font_path, const Import_configuration& in_configuration)
{
#define ABORT(msg) { puts(msg); return 1; }

	int result = 0;
	Import_configuration config = in_configuration;
	int fixedWidth = -1, fixedHeight = -1;

	msdf_atlas::GeneratorAttributes generatorAttributes;
	generatorAttributes.overlapSupport = true;
	generatorAttributes.scanlinePass = true;
	generatorAttributes.errorCorrectionThreshold = MSDFGEN_DEFAULT_ERROR_CORRECTION_THRESHOLD;

	double minEmSize = 0;
	enum {
		/// Range specified in EMs
		RANGE_EM,
		/// Range specified in output pixels
		RANGE_PIXEL,
	} rangeMode = RANGE_PIXEL;
	double rangeValue = 0;
	msdf_atlas::TightAtlasPacker::DimensionsConstraint atlasSizeConstraint = msdf_atlas::TightAtlasPacker::DimensionsConstraint::MULTIPLE_OF_FOUR_SQUARE;

	// Fix up configuration based on related values
	if (!(config.imageType == ImageType::PSDF || config.imageType == ImageType::MSDF || config.imageType == ImageType::MTSDF))
		config.miterLimit = 0;
	if (config.emSize > minEmSize)
		minEmSize = config.emSize;

	if (!(config.imageType == ImageType::SDF || config.imageType == ImageType::PSDF || config.imageType == ImageType::MSDF || config.imageType == ImageType::MTSDF)) {
		rangeMode = RANGE_PIXEL;
		rangeValue = (double)(config.imageType == ImageType::SOFT_MASK);
	}
	else if (rangeValue <= 0) {
		rangeMode = RANGE_PIXEL;
		rangeValue = 2.0;
	}

	// Load font
	struct FontHolder {
		msdfgen::FreetypeHandle* ft;
		msdfgen::FontHandle* font;

		explicit FontHolder(const char* fontFilename) : ft(nullptr), font(nullptr) {
			if ((ft = msdfgen::initializeFreetype()))
				font = msdfgen::loadFont(ft, fontFilename);
		}
		~FontHolder() {
			if (ft) {
				if (font)
					msdfgen::destroyFont(font);
				msdfgen::deinitializeFreetype(ft);
			}
		}
		operator msdfgen::FontHandle* () const {
			return font;
		}
	} font(in_font_path.c_str());
	if (!font)
		ABORT("Failed to load specified font file.");
	msdfgen::FontMetrics fontMetrics = { };
	msdfgen::getFontMetrics(fontMetrics, font);
	if (fontMetrics.emSize <= 0)
		fontMetrics.emSize = 32.0;

	// Load character set
	msdf_atlas::Charset charset = msdf_atlas::Charset::ASCII;

	// Load glyphs
	std::vector<msdf_atlas::GlyphGeometry> glyphs;
	loadGlyphs(glyphs, font, charset);
	printf("Loaded geometry of %d out of %d characters.\n", (int)glyphs.size(), (int)charset.size());

	// Determine final atlas dimensions, scale and range, pack glyphs
	{
		double unitRange = 0, pxRange = 0;
		switch (rangeMode) {
		case RANGE_EM:
			unitRange = rangeValue * fontMetrics.emSize;
			break;
		case RANGE_PIXEL:
			pxRange = rangeValue;
			break;
		}
		bool fixedDimensions = fixedWidth >= 0 && fixedHeight >= 0;
		bool fixedScale = config.emSize > 0;
		msdf_atlas::TightAtlasPacker atlasPacker;
		if (fixedDimensions)
			atlasPacker.setDimensions(fixedWidth, fixedHeight);
		else
			atlasPacker.setDimensionsConstraint(atlasSizeConstraint);
		atlasPacker.setPadding(config.imageType == ImageType::MSDF || config.imageType == ImageType::MTSDF ? 0 : -1);
		// TODO: In this case (if padding is -1), the border pixels of each glyph are black, but still computed. For floating-point output, this may play a role.
		if (fixedScale)
			atlasPacker.setScale(config.emSize / fontMetrics.emSize);
		else
			atlasPacker.setMinimumScale(minEmSize / fontMetrics.emSize);
		atlasPacker.setPixelRange(pxRange);
		atlasPacker.setUnitRange(unitRange);
		atlasPacker.setMiterLimit(config.miterLimit);
		if (int remaining = atlasPacker.pack(glyphs.data(), glyphs.size())) {
			if (remaining < 0) {
				ABORT("Failed to pack glyphs into atlas.");
			}
			else {
				printf("Error: Could not fit %d out of %d glyphs into the atlas.\n", remaining, (int)glyphs.size());
				return 1;
			}
		}
		atlasPacker.getDimensions(config.width, config.height);
		if (!(config.width > 0 && config.height > 0))
			ABORT("Unable to determine atlas size.");
		config.emSize = atlasPacker.getScale() * fontMetrics.emSize;
		config.pxRange = atlasPacker.getPixelRange();
		if (!fixedScale)
			printf("Glyph size: %.9g pixels/EM\n", config.emSize);
		if (!fixedDimensions)
			printf("Atlas dimensions: %d x %d\n", config.width, config.height);
	}

	// Generate atlas bitmap
	// Edge coloring
	if (config.imageType == ImageType::MSDF || config.imageType == ImageType::MTSDF) {
		unsigned long long glyphSeed = config.coloringSeed;
		for (msdf_atlas::GlyphGeometry& glyph : glyphs) {
			glyphSeed *= 6364136223846793005ull;
			glyph.edgeColoring(config.angleThreshold, glyphSeed);
		}
	}

	msdf_atlas::ImmediateAtlasGenerator<float, 3, msdf_atlas::msdfGenerator, msdf_atlas::BitmapAtlasStorage<float, 3> > generator(config.width, config.height);
	generator.setAttributes(generatorAttributes);
	generator.setThreadCount(config.threadCount);
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
		;
		msdf_atlas::GlyphBox gb = glyph;
		new_glyph.m_atlas_pixel_position.x = gb.rect.x;
		new_glyph.m_atlas_pixel_position.y = gb.rect.y;

		new_glyph.m_offset_to_glyph = glm::vec2{ glyph.getBoxTranslate().x, -gb.rect.h + glyph.getBoxTranslate().y };

		new_glyph.m_atlas_pixel_size.x = gb.rect.w;
		new_glyph.m_atlas_pixel_size.y = gb.rect.h;

		new_glyph.m_pixel_advance = gb.advance;
	}

	m_max_glyph_height = fontMetrics.ascenderY - fontMetrics.descenderY;
	
	return true;
}

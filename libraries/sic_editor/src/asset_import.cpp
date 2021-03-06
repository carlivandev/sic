#include "sic/editor/asset_import.h"

#include "sic/shader_parser.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include "msdf-atlas-gen/msdf-atlas-gen.h"

#include <filesystem>

namespace sic_private
{
	using namespace sic;

	Asset_model::Mesh process_mesh(aiMesh* in_mesh, const aiScene* in_scene)
	{
		Asset_model::Mesh new_mesh;
		new_mesh.m_positions.resize(in_mesh->mNumVertices);
		new_mesh.m_normals.resize(in_mesh->mNumVertices);
		new_mesh.m_texcoords.resize(in_mesh->mNumVertices);
		new_mesh.m_tangents.resize(in_mesh->mNumVertices);
		new_mesh.m_bitangents.resize(in_mesh->mNumVertices);

		for (ui16 i = 0; i < in_mesh->mNumVertices; i++)
		{
			glm::vec3 vert_pos;
			vert_pos.x = in_mesh->mVertices[i].x;
			vert_pos.y = in_mesh->mVertices[i].y;
			vert_pos.z = in_mesh->mVertices[i].z;
			new_mesh.m_positions[i] = vert_pos;

			glm::vec3 vert_normal;
			vert_normal.x = in_mesh->mNormals[i].x;
			vert_normal.y = in_mesh->mNormals[i].y;
			vert_normal.z = in_mesh->mNormals[i].z;
			new_mesh.m_normals[i] = vert_normal;
			
			if (in_mesh->mTextureCoords[0])
			{
				glm::vec2 vert_texture_coord;
				vert_texture_coord.x = in_mesh->mTextureCoords[0][i].x;
				vert_texture_coord.y = in_mesh->mTextureCoords[0][i].y;
				new_mesh.m_texcoords[i] = vert_texture_coord;
			}
			else
				new_mesh.m_texcoords[i] = glm::vec2(0.0f, 0.0f);
		}

		// process indices
		for (ui32 i = 0; i < in_mesh->mNumFaces; i++)
		{
			aiFace& face = in_mesh->mFaces[i];

			assert(face.mNumIndices == 3 && "Only triangles supported!");

			new_mesh.m_indices.push_back(face.mIndices[0]);
			new_mesh.m_indices.push_back(face.mIndices[1]);
			new_mesh.m_indices.push_back(face.mIndices[2]);

			const glm::vec3& pos_0 = new_mesh.m_positions[face.mIndices[0]];
			const glm::vec3& pos_1 = new_mesh.m_positions[face.mIndices[1]];
			const glm::vec3& pos_2 = new_mesh.m_positions[face.mIndices[2]];

			const glm::vec2& uv_0 = new_mesh.m_texcoords[face.mIndices[0]];
			const glm::vec2& uv_1 = new_mesh.m_texcoords[face.mIndices[1]];
			const glm::vec2& uv_2 = new_mesh.m_texcoords[face.mIndices[2]];

			const glm::vec3 edge1 = pos_1 - pos_0;
			const glm::vec3 edge2 = pos_2 - pos_0;
			const glm::vec2 deltaUV1 = uv_1 - uv_0;
			const glm::vec2 deltaUV2 = uv_2 - uv_0;

			const float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

			glm::vec3 tangent;
			glm::vec3 bitangent;

			tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
			tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
			tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

			bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
			bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
			bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

			new_mesh.m_tangents[face.mIndices[0]] = new_mesh.m_tangents[face.mIndices[1]] = new_mesh.m_tangents[face.mIndices[2]] = tangent;
			new_mesh.m_bitangents[face.mIndices[0]] = new_mesh.m_bitangents[face.mIndices[1]] = new_mesh.m_bitangents[face.mIndices[2]] = bitangent;
		}

		// process material
		if (in_mesh->mMaterialIndex >= 0)
		{
			aiMaterial* material = in_scene->mMaterials[in_mesh->mMaterialIndex];

			//we only store slot names
			//model then has LUT from material_slot to material, so its the materials that own textures
			new_mesh.m_material_slot = material->GetName().C_Str();
		}

		return std::move(new_mesh);
	}

	void process_node(aiNode* in_node, const aiScene* in_scene, std::vector<Asset_model::Mesh>& out_meshes)
	{
		// process all the node's meshes (if any)
		for (unsigned int i = 0; i < in_node->mNumMeshes; i++)
		{
			aiMesh* mesh = in_scene->mMeshes[in_node->mMeshes[i]];

			Asset_model::Mesh&& processed_mesh = process_mesh(mesh, in_scene);

			out_meshes.emplace_back(std::move(processed_mesh));
		}

		// then do the same for each of its children
		for (unsigned int i = 0; i < in_node->mNumChildren; i++)
			process_node(in_node->mChildren[i], in_scene, out_meshes);
	}
}

bool sic::import_model(Asset_model& inout_model, std::string&& in_data)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFileFromMemory(in_data.data(), in_data.size(), aiProcess_Triangulate | aiProcess_FlipUVs);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		SIC_LOG_E(g_log_asset, "Assimp error: {0}", importer.GetErrorString());
		return false;
	}

	sic_private::process_node(scene->mRootNode, scene, inout_model.m_meshes);

	if (inout_model.m_meshes.empty())
		return false;

	for (ui32 i = 0; i < inout_model.m_meshes.size(); i++)
		inout_model.m_materials.emplace_back();

	return true;
}

bool sic::import_material(Asset_material& inout_material, Temporary_string&& in_vertex_shader_path, Temporary_string&& in_fragment_shader_path)
{
	inout_material.m_vertex_shader_path = in_vertex_shader_path.get_c_str();
	inout_material.m_fragment_shader_path = in_fragment_shader_path.get_c_str();
	
	inout_material.m_vertex_shader_code = Shader_parser::parse_shader(std::move(in_vertex_shader_path)).value_or("");
	inout_material.m_fragment_shader_code = Shader_parser::parse_shader(std::move(in_fragment_shader_path)).value_or("");

	return true;
}

bool sic::import_texture(Asset_texture& inout_texture, std::string&& in_data, bool in_free_texture_data_after_setup, bool in_flip_vertically)
{
	if (in_data.empty())
	{
		SIC_LOG_E(g_log_asset, "Failed to import texture, no data provided!");
		return false;
	}

	sic::i32 width, height, channel_count;

	stbi_set_flip_vertically_on_load(in_flip_vertically);
	stbi_uc* texture_data = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(in_data.c_str()), static_cast<int>(in_data.size()), &width, &height, &channel_count, 0);

	if (!texture_data)
	{
		SIC_LOG_E(g_log_asset, "Failed to import texture, data was invalid!");
		return false;
	}

	inout_texture.m_texture_data = std::unique_ptr<unsigned char>(texture_data);


	auto is_power_of_2 = [](i32 in_value) { if (in_value == 0) return false; return (in_value & (in_value - 1)) == 0; };

	if (!is_power_of_2(width) || !is_power_of_2(height))
	{
		SIC_LOG_E(g_log_asset, "Failed to import texture, dimensions were not power of 2!");
		return false;
	}


	inout_texture.m_width = width;
	inout_texture.m_height = height;
	inout_texture.m_format = static_cast<Texture_format>(channel_count);
	inout_texture.m_texture_data_bytesize = channel_count * inout_texture.m_width * inout_texture.m_height;
	inout_texture.m_free_texture_data_after_setup = in_free_texture_data_after_setup;

	return true;
}

bool sic::import_font(Asset_font& inout_font, const std::string& in_font_path, Asset_font::Import_configuration in_configuration)
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
	if (!(in_configuration.m_image_type == Asset_font::Image_type::PSDF || in_configuration.m_image_type == Asset_font::Image_type::MSDF || in_configuration.m_image_type == Asset_font::Image_type::MTSDF))
		in_configuration.m_miter_limit = 0;
	if (in_configuration.m_em_size > min_em_size)
		min_em_size = in_configuration.m_em_size;

	if (!(in_configuration.m_image_type == Asset_font::Image_type::SDF || in_configuration.m_image_type == Asset_font::Image_type::PSDF || in_configuration.m_image_type == Asset_font::Image_type::MSDF || in_configuration.m_image_type == Asset_font::Image_type::MTSDF))
	{
		range_mode = RANGE_PIXEL;
		range_value = (double)(in_configuration.m_image_type == Asset_font::Image_type::SOFT_MASK);
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
			m_ft = msdfgen::initializeFreetype();
			if (m_ft)
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
		atlas_packer.setPadding(in_configuration.m_image_type == Asset_font::Image_type::MSDF || in_configuration.m_image_type == Asset_font::Image_type::MTSDF ? 0 : -1);
		// TODO: In this case (if padding is -1), the border pixels of each glyph are black, but still computed. For floating-point output, this may play a role.
		if (uses_fixed_scale)
			atlas_packer.setScale(in_configuration.m_em_size / font_metrics.emSize);
		else
			atlas_packer.setMinimumScale(min_em_size / font_metrics.emSize);
		atlas_packer.setPixelRange(px_range);
		atlas_packer.setUnitRange(unit_range);
		atlas_packer.setMiterLimit(in_configuration.m_miter_limit);

		if (int remaining = atlas_packer.pack(glyphs.data(), (int)glyphs.size()))
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
	if (in_configuration.m_image_type == Asset_font::Image_type::MSDF || in_configuration.m_image_type == Asset_font::Image_type::MTSDF)
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
	generator.resize(1024, 1024);
	generator.generate(glyphs.data(), (int)glyphs.size());
	msdfgen::BitmapConstRef<float, 3> bitmap = (msdfgen::BitmapConstRef<float, 3>) generator.atlasStorage();

	inout_font.m_atlas_data_bytesize = 3 * bitmap.width * bitmap.height * sizeof(float);
	inout_font.m_atlas_data = std::unique_ptr<unsigned char>(new unsigned char[inout_font.m_atlas_data_bytesize]);
	memcpy(inout_font.m_atlas_data.get(), bitmap.pixels, inout_font.m_atlas_data_bytesize);

	inout_font.m_width = bitmap.width;
	inout_font.m_height = bitmap.height;
	inout_font.m_format = Texture_format::rgb;

	for (const msdf_atlas::GlyphGeometry& glyph : glyphs)
	{
		if (inout_font.m_glyphs.size() <= glyph.getCodepoint())
			inout_font.m_glyphs.resize(glyph.getCodepoint() + 1);

		auto& new_glyph = inout_font.m_glyphs[glyph.getCodepoint()];

		msdf_atlas::GlyphBox gb = glyph;
		new_glyph.m_atlas_pixel_position.x = (float)gb.rect.x;
		new_glyph.m_atlas_pixel_position.y = (float)gb.rect.y;

		new_glyph.m_offset_to_glyph = glm::vec2{ -glyph.getBoxTranslate().x, -gb.rect.h + glyph.getBoxTranslate().y };

		new_glyph.m_atlas_pixel_size.x = (float)gb.rect.w;
		new_glyph.m_atlas_pixel_size.y = (float)gb.rect.h;

		new_glyph.m_pixel_advance = (float)(glyph.getAdvance() * (in_configuration.m_em_size / font_metrics.emSize));
	}

	for (auto& glyph : inout_font.m_glyphs)
		glyph.m_kerning_values.resize(inout_font.m_glyphs.size());

	for (const msdf_atlas::GlyphGeometry& glyph_left : glyphs)
	{
		for (const msdf_atlas::GlyphGeometry& glyph_right : glyphs)
		{
			if (&glyph_left == &glyph_right)
				continue;

			auto& kerning_to_modify = inout_font.m_glyphs[glyph_right.getCodepoint()].m_kerning_values[glyph_left.getCodepoint()];

			double kerning;
			if (msdfgen::getKerning(kerning, font.m_font, glyph_left.getCodepoint(), glyph_right.getCodepoint()))
				kerning_to_modify = (float)kerning;
			else
				kerning_to_modify = 0.0f;
		}
	}

	inout_font.m_max_glyph_height = (float)(font_metrics.ascenderY - font_metrics.descenderY);
	inout_font.m_baseline = (float)font_metrics.lineHeight;
	inout_font.m_em_size = (float)in_configuration.m_em_size;

	//msdf_atlas::saveImage(bitmap, msdf_atlas::ImageFormat::PNG, "test.png");

	return true;
}
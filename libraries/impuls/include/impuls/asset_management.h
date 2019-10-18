#pragma once
#include <string>
#include <memory>

namespace impuls
{
	struct asset_material;

	struct asset_texture;
	enum class e_texture_format;

	struct asset_model;

	namespace asset_management
	{
		std::shared_ptr<asset_material> load_material(const char* in_filepath);

		std::shared_ptr<asset_texture> load_texture(const char* in_filepath, e_texture_format in_format);

		std::shared_ptr<asset_model> load_model(const char* in_filepath);
	}
}
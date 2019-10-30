#pragma once
#include "impuls/serializer.h"
#include "impuls/asset_types.h"
#include "impuls/file_management.h"

#include "nlohmann/json.hpp"

#include <string>
#include <memory>

namespace impuls
{
	namespace asset_management
	{
		std::shared_ptr<asset_material> load_material(const char* in_filepath);

		std::shared_ptr<asset_texture> load_texture(const char* in_filepath, e_texture_format in_format);

		std::shared_ptr<asset_model> load_model(const char* in_filepath);
	}
}
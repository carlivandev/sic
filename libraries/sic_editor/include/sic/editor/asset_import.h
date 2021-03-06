#pragma once
#include "sic/core/temporary_string.h"

#include "sic/asset_model.h"
#include "sic/asset_material.h"
#include "sic/asset_texture.h"
#include "sic/asset_font.h"

#include <string>

namespace sic
{
	bool import_model(Asset_model& inout_model, std::string&& in_data);
	bool import_material(Asset_material& inout_material, Temporary_string&& in_vertex_shader_path, Temporary_string&& in_fragment_shader_path);
	bool import_texture(Asset_texture& inout_texture, std::string&& in_data, bool in_free_texture_data_after_setup, bool in_flip_vertically);
	bool import_font(Asset_font& inout_font, const std::string& in_font_path, Asset_font::Import_configuration in_configuration);
}
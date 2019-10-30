#pragma once
#include "impuls/defines.h"
#include "impuls/gl_includes.h"
#include "impuls/asset.h"
#include "impuls/asset_texture.h"

#include <memory>

namespace impuls
{
	struct asset_material : i_asset<true, true>
	{
		void load(const state_assetsystem& in_assetsystem_state, deserialize_stream& in_stream) override;
		void save(const state_assetsystem& in_assetsystem_state, serialize_stream& out_stream) const override;

		struct texture_parameter
		{
			std::string m_name;
			asset_ref<asset_texture> m_texture;
		};

		std::string m_vertex_shader;
		std::string m_fragment_shader;

		std::vector<texture_parameter> m_texture_parameters;

		GLuint m_program_id = 0;
	};
}
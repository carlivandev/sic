#pragma once
#include "impuls/defines.h"
#include "impuls/gl_includes.h"
#include "impuls/asset.h"
#include "impuls/asset_texture.h"

#include <memory>

namespace impuls
{
	struct Asset_material : Asset
	{
		bool has_post_load() override final { return true; }
		bool has_pre_unload() override final { return true; }

		void load(const State_assetsystem& in_assetsystem_state, Deserialize_stream& in_stream) override;
		void save(const State_assetsystem& in_assetsystem_state, Serialize_stream& out_stream) const override;

		struct Texture_parameter
		{
			std::string m_name;
			Asset_ref<Asset_texture> m_texture;
		};

		std::string m_vertex_shader;
		std::string m_fragment_shader;

		std::vector<Texture_parameter> m_texture_parameters;

		GLuint m_program_id = 0;
	};
}
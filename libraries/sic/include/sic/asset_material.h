#pragma once
#include "sic/defines.h"
#include "sic/asset.h"
#include "sic/asset_texture.h"
#include "sic/opengl_program.h"

#include <optional>
namespace sic
{
	enum class Material_blend_mode
	{
		Opaque,
		Masked,
		Translucent,
		Additive,
		Invalid
	};

	struct Asset_material : Asset
	{
		struct Texture_parameter
		{
			std::string m_name;
			Asset_ref<Asset_texture> m_texture;
		};

		bool has_post_load() override final { return true; }
		bool has_pre_unload() override final { return true; }

		void load(const State_assetsystem& in_assetsystem_state, Deserialize_stream& in_stream) override;
		void save(const State_assetsystem& in_assetsystem_state, Serialize_stream& out_stream) const override;
		bool import(const std::string& in_vertex_shader_path, const std::string& in_fragment_shader_path);

		void get_dependencies(Asset_dependency_gatherer& out_dependencies) override;

		std::string m_vertex_shader_path;
		std::string m_fragment_shader_path;

		std::string m_vertex_shader_code;
		std::string m_fragment_shader_code;

		std::vector<Texture_parameter> m_texture_parameters;

		std::optional<OpenGl_program> m_program;
		Material_blend_mode m_blend_mode = Material_blend_mode::Opaque;
	};
}
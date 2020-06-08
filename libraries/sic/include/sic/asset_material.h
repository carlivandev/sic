#pragma once
#include "sic/defines.h"
#include "sic/asset.h"
#include "sic/asset_texture.h"
#include "sic/opengl_program.h"
#include "sic/temporary_string.h"
#include "sic/opengl_uniform_block.h"

#include <optional>
#include <vector>

namespace sic
{
	struct Material_instance_data_layout
	{
		friend struct Asset_material;

		struct Layout_entry
		{
			std::string m_name;
			ui32 m_bytesize = 0;
		};

		template <typename T_data_type>
		void add_entry(const char* in_name)
		{
			GLuint bytesize = uniform_block_alignment_functions::get_alignment<T_data_type>();
			m_entries.push_back({ in_name, bytesize });
			m_bytesize += bytesize;
		}

	private:
		std::vector<Layout_entry> m_entries;
		ui32 m_bytesize = 0;
	};

	enum class Material_blend_mode
	{
		Opaque,
		Masked,
		Translucent,
		Additive,
		Invalid
	};

	struct Material_texture_parameter
	{
		std::string m_name;
		Asset_ref<Asset_texture> m_texture;
		bool m_overriden = false;
	};

	struct Material_parameters
	{
		std::vector<Material_texture_parameter> m_textures;
	};

	struct Asset_material : Asset
	{
		bool has_post_load() override final { return true; }
		bool has_pre_unload() override final { return true; }

		void load(const State_assetsystem& in_assetsystem_state, Deserialize_stream& in_stream) override;
		void save(const State_assetsystem& in_assetsystem_state, Serialize_stream& out_stream) const override;
		bool import(Temporary_string&& in_vertex_shader_path, Temporary_string&& in_fragment_shader_path);

		void get_dependencies(Asset_dependency_gatherer& out_dependencies) override;

		void enable_instancing(ui32 in_max_elements_per_drawcall, const char* in_instance_block_name, const Material_instance_data_layout& in_data_layout);
		void disable_instancing();

		size_t add_instance_data();
		void remove_instance_data(size_t in_to_remove);

		void set_parent(Asset_material* in_parent);
		void set_texture_parameter(const char* in_name, const Asset_ref<Asset_texture>& in_texture);
		void set_texture_parameter_internal(const char* in_name, const Asset_ref<Asset_texture>& in_texture, bool in_should_override);

		Asset_ref<Asset_material> m_parent;
		Asset_ref<Asset_material> m_outermost_parent;
		std::vector<Asset_ref<Asset_material>> m_children;

		std::string m_vertex_shader_path;
		std::string m_fragment_shader_path;

		std::string m_vertex_shader_code;
		std::string m_fragment_shader_code;

		Material_parameters m_parameters;

		std::optional<OpenGl_program> m_program;
		Material_blend_mode m_blend_mode = Material_blend_mode::Opaque;

		std::optional<OpenGl_uniform_block> m_instance_data_uniform_block;
		std::optional<OpenGl_texture> m_instance_data_texture;
		std::vector<char> m_instance_buffer;
		std::vector<size_t> m_free_instance_buffer_indices;
		std::string m_instance_block_name;
		std::unordered_map<std::string, ui32> m_instance_data_name_to_offset_lut;
		ui32 m_max_elements_per_drawcall = 0;
		ui32 m_instance_stride = 0;
		ui32 m_instance_vec4_stride = 0;
		bool m_is_instanced = false;
		On_asset_loaded::Handle m_on_parent_loaded_handle;

	private:
		void set_outermost_parent();
	};
}
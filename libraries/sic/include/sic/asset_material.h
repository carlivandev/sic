#pragma once
#include "sic/asset.h"
#include "sic/asset_texture.h"
#include "sic/opengl_program.h"
#include "sic/opengl_uniform_block.h"

#include "sic/core/defines.h"
#include "sic/core/temporary_string.h"

#include <optional>
#include <vector>
#include <unordered_set>

namespace sic
{
	struct Material_data_layout
	{
		friend struct Asset_material;

		struct Layout_entry
		{
			std::function<void(Asset_material&)> m_set_default_value_func;
			std::string m_name;
			ui32 m_bytesize = 0;
		};

		void add_entry_texture(const char* in_name, Asset_ref<Asset_texture_base> in_default_value);
		void add_entry_vec4(const char* in_name, const glm::vec4& in_default_value);
		void add_entry_mat4(const char* in_name, const glm::mat4& in_default_value);

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
		Asset_ref<Asset_texture_base> m_value;
		bool m_overriden = false;
	};

	struct Material_vec4_parameter
	{
		std::string m_name;
		glm::vec4 m_value;
		bool m_overriden = false;
	};

	struct Material_mat4_parameter
	{
		std::string m_name;
		glm::mat4 m_value;
		bool m_overriden = false;
	};

	struct Material_parameters
	{
		std::vector<Material_texture_parameter>& get_textures() { return std::get<std::vector<Material_texture_parameter>>(m_parameters); }
		const std::vector<Material_texture_parameter>& get_textures() const { return std::get<std::vector<Material_texture_parameter>>(m_parameters); }
		
		std::vector<Material_vec4_parameter>& get_vec4s() { return std::get<std::vector<Material_vec4_parameter>>(m_parameters); }
		const std::vector<Material_vec4_parameter>& get_vec4s() const { return std::get<std::vector<Material_vec4_parameter>>(m_parameters); }

		std::vector<Material_mat4_parameter>& get_mat4s() { return std::get<std::vector<Material_mat4_parameter>>(m_parameters); }
		const std::vector<Material_mat4_parameter>& get_mat4s() const { return std::get<std::vector<Material_mat4_parameter>>(m_parameters); }

		std::tuple<std::vector<Material_texture_parameter>, std::vector<Material_vec4_parameter>, std::vector<Material_mat4_parameter>> m_parameters;
	};

	struct On_material_modified : Delegate<Asset_material*> {};

	struct Asset_material : Asset
	{
		bool has_post_load() override final { return true; }
		bool has_pre_unload() override final { return true; }

		void load(const State_assetsystem& in_assetsystem_state, Deserialize_stream& in_stream) override;
		void save(const State_assetsystem& in_assetsystem_state, Serialize_stream& out_stream) const override;

		void get_dependencies(Asset_dependency_gatherer& out_dependencies) override;

		void initialize(ui32 in_max_elements_per_drawcall, const Material_data_layout& in_data_layout);

		void set_is_instanced(bool in_instanced);

		size_t add_instance_data();
		void remove_instance_data(size_t in_to_remove);

		void set_parent(Asset_material* in_parent);
		void set_texture_parameter(const char* in_name, const Asset_ref<Asset_texture_base>& in_texture);
		void set_vec4_parameter(const char* in_name, const glm::vec4& in_vec4);
		void set_mat4_parameter(const char* in_name, const glm::mat4& in_mat4);

		void set_parameters_to_default_on_instance(size_t in_instance_index);
		bool set_parameter_on_instance(const char* in_name, const Asset_ref<Asset_texture_base>& in_texture, size_t in_instance_index);
		bool set_parameter_on_instance(const char* in_name, const glm::vec4& in_vec4, size_t in_instance_index);

		void add_default_instance_data();

		void on_modified();

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
		bool m_two_sided = false;

		std::optional<OpenGl_texture> m_instance_data_texture;
		std::optional<OpenGl_buffer> m_instance_data_buffer;
		std::vector<char> m_instance_buffer;
		std::vector<size_t> m_free_instance_buffer_indices;
		std::unordered_set<size_t> m_active_instance_indices;
		std::unordered_map<std::string, ui32> m_instance_data_name_to_offset_lut;
		ui32 m_max_elements_per_drawcall = 0;
		ui32 m_instance_vec4_stride = 0;
		bool m_is_instanced = false;
		On_asset_loaded::Handle m_on_parent_loaded_handle;
		On_material_modified m_on_modified_delegate;
	private:
		size_t add_instance_data_internal(bool in_should_set_parameters);
		void remove_instance_data_internal(size_t in_to_remove);

		void set_outermost_parent();

		template<typename T_list_element_type, typename T_parameter_type>
		void set_parameter_internal(const char* in_name, const T_parameter_type& in_value, bool in_should_override);

		std::unordered_map<std::string, Asset_ref<Asset_texture_base>> m_texture_parameters_to_apply;
	};

	template<typename T_list_element_type, typename T_parameter_type>
	inline void Asset_material::set_parameter_internal(const char* in_name, const T_parameter_type& in_value, bool in_should_override)
	{
		auto& param_list = std::get<std::vector<T_list_element_type>>(m_parameters.m_parameters);

		if (m_parent.is_valid())
		{
			for (auto& param : param_list)
			{
				if (param.m_name == in_name)
				{
					if (param.m_overriden && !in_should_override)
						return;

					param.m_value = in_value;
					param.m_overriden = true;
				}
			}
		}
		else
		{
			bool was_set = false;
			for (auto& param : param_list)
			{
				if (param.m_name == in_name)
				{
					param.m_value = in_value;
					was_set = true;
				}
			}

			if (!was_set)
				param_list.push_back({ in_name, in_value });
		}

		for (auto& child : m_children)
		{
			if (!child.is_valid())
				continue;

			child.get_mutable()->set_parameter_internal<T_list_element_type, T_parameter_type>(in_name, in_value, false);
		}

		on_modified();
	}
}
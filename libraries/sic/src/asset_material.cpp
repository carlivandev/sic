#pragma once
#include "sic/asset_material.h"
#include "sic/system_asset.h"

#include "sic/core/defines.h"
#include "sic/core/magic_enum.h"
#include "sic/core/logger.h"

#include <unordered_set>
#include "sic/material_dynamic_parameters.h"
#include "sic/system_model.h"

void sic::Asset_material::load(const State_assetsystem& in_assetsystem_state, Deserialize_stream& in_stream)
{
	m_parent.deserialize(in_assetsystem_state, in_stream);

	size_t child_count = 0;
	in_stream.read(child_count);

	for (size_t i = 0; i < child_count; i++)
	{
		Asset_ref<Asset_material> child;
		child.deserialize(in_assetsystem_state, in_stream);
		m_children.push_back(child);
	}

	in_stream.read(m_vertex_shader_path);
	in_stream.read(m_fragment_shader_path);

	m_blend_mode = Material_blend_mode::Invalid;
	in_stream.read(m_blend_mode);
	in_stream.read(m_two_sided);

	in_stream.read(m_vertex_shader_code);
	in_stream.read(m_fragment_shader_code);

	in_stream.read(m_is_instanced);
	in_stream.read(m_max_elements_per_drawcall);
	in_stream.read(m_instance_vec4_stride);
	in_stream.read(m_instance_data_name_to_offset_lut);

	const ui32 stride = m_instance_vec4_stride * uniform_block_alignment_functions::get_alignment<glm::vec4>();
	m_instance_buffer.reserve((ui64)m_max_elements_per_drawcall * stride);

	size_t tex_count = 0;
	in_stream.read(tex_count);

	for (size_t i = 0; i < tex_count; i++)
	{
		auto& param = m_parameters.get_textures().emplace_back();
		in_stream.read(param.m_name);
		param.m_value.deserialize(in_assetsystem_state, in_stream);
	}

	size_t vec4_count = 0;
	in_stream.read(vec4_count);

	for (size_t i = 0; i < vec4_count; i++)
	{
		auto& param = m_parameters.get_vec4s().emplace_back();
		in_stream.read(param.m_name);
		in_stream.read(param.m_value);
	}
}

void sic::Asset_material::save(const State_assetsystem& in_assetsystem_state, Serialize_stream& out_stream) const
{
	in_assetsystem_state;

	m_parent.serialize(out_stream);

	out_stream.write(m_children.size());

	for (auto& child : m_children)
		child.serialize(out_stream);

	out_stream.write(m_vertex_shader_path);
	out_stream.write(m_fragment_shader_path);

	out_stream.write(m_blend_mode);
	out_stream.write(m_two_sided);

	out_stream.write(m_vertex_shader_code);
	out_stream.write(m_fragment_shader_code);

	out_stream.write(m_is_instanced);
	out_stream.write(m_max_elements_per_drawcall);
	out_stream.write(m_instance_vec4_stride);
	out_stream.write(m_instance_data_name_to_offset_lut);

	out_stream.write(m_parameters.get_textures().size());

	for (const auto& param : m_parameters.get_textures())
	{
		out_stream.write(param.m_name);
		param.m_value.serialize(out_stream);
	}

	out_stream.write(m_parameters.get_vec4s().size());

	for (const auto& param : m_parameters.get_vec4s())
	{
		out_stream.write(param.m_name);
		out_stream.write(param.m_value);
	}
}

void sic::Asset_material::get_dependencies(Asset_dependency_gatherer& out_dependencies)
{
	for (auto& tex_param : m_parameters.get_textures())
		out_dependencies.add_dependency(*this, tex_param.m_value);

	if (m_parent.is_valid())
		out_dependencies.add_dependency(*this, m_parent);
}

void sic::Asset_material::initialize(ui32 in_max_elements_per_drawcall, const Material_data_layout& in_data_layout)
{
	m_instance_buffer.reserve((ui64)in_max_elements_per_drawcall * in_data_layout.m_bytesize);
	m_max_elements_per_drawcall = in_max_elements_per_drawcall;
	m_instance_vec4_stride =  (ui32)(std::ceil((float)in_data_layout.m_bytesize / (float)uniform_block_alignment_functions::get_alignment<glm::vec4>()));

	ui32 offset = 0;
	for (auto&& entry : in_data_layout.m_entries)
	{
		m_instance_data_name_to_offset_lut[entry.m_name] = offset;
		offset += entry.m_bytesize;
	}

	for (auto&& entry : in_data_layout.m_entries)
		entry.m_set_default_value_func(*this);

	on_modified();
}

void sic::Asset_material::set_is_instanced(bool in_instanced)
{
	m_is_instanced = in_instanced;
	on_modified();
}

size_t sic::Asset_material::add_instance_data()
{
	const size_t idx = add_instance_data_internal(true);
	m_active_instance_indices.insert(idx);
	return idx;
}

void sic::Asset_material::remove_instance_data(size_t in_to_remove)
{
	m_active_instance_indices.erase(in_to_remove);

	if (m_outermost_parent.is_valid())
		m_outermost_parent.get_mutable()->remove_instance_data_internal(in_to_remove);
	else
		remove_instance_data_internal(in_to_remove);
}

void sic::Asset_material::set_parent(Asset_material* in_parent)
{
	if (m_parent.get() == in_parent)
		return;
	
	if (!in_parent && m_parent.is_valid())
	{
		//unchild from old parent

		auto& children_list = m_parent.get_mutable()->m_children;
		children_list.erase
		(
			std::remove_if
			(
				children_list.begin(), children_list.end(),
				[this](const Asset_ref<Asset_material>& in_mat) { return in_mat.get() == this; }
			),
			children_list.end()
		);

		m_outermost_parent = m_parent = Asset_ref<Asset_material>();
		add_default_instance_data();
		return;
	}

	m_parent = Asset_ref<Asset_material>(in_parent);

	if (m_parent.is_valid())
	{
		m_instance_buffer.clear();
		in_parent->m_children.push_back(Asset_ref<Asset_material>(this));

		m_parameters = in_parent->m_parameters;

		for (auto&& param : m_parameters.get_textures())
			param.m_overriden = false;

		for (auto&& param : m_parameters.get_vec4s())
			param.m_overriden = false;

		m_blend_mode = in_parent->m_blend_mode;

		set_outermost_parent();
	}
	else
	{
		m_parameters.get_textures().clear();
		m_parameters.get_vec4s().clear();
	}

	on_modified();
}

void sic::Asset_material::set_texture_parameter(const char* in_name, const Asset_ref<Asset_texture_base>& in_texture)
{
	set_parameter_internal<Material_texture_parameter>(in_name, in_texture, true);
}

void sic::Asset_material::set_vec4_parameter(const char* in_name, const glm::vec4& in_vec4)
{
	set_parameter_internal<Material_vec4_parameter>(in_name, in_vec4, true);
}

void sic::Asset_material::set_mat4_parameter(const char* in_name, const glm::mat4& in_mat4)
{
	set_parameter_internal<Material_mat4_parameter>(in_name, in_mat4, true);
}

void sic::Asset_material::set_parameters_to_default_on_instance(size_t in_instance_index)
{
	for (auto& param : m_parameters.get_textures())
		set_parameter_on_instance(param.m_name.c_str(), param.m_value, in_instance_index);

	for (auto& param : m_parameters.get_vec4s())
		set_parameter_on_instance(param.m_name.c_str(), param.m_value, in_instance_index);
}

bool sic::Asset_material::set_parameter_on_instance(const char* in_name, const Asset_ref<Asset_texture_base>& in_texture, size_t in_instance_index)
{
	if (m_outermost_parent.is_valid())
	{
		m_outermost_parent.get_mutable()->set_parameter_on_instance(in_name, in_texture, in_instance_index);
		return true;
	}
	
	if (m_instance_buffer.size() <= in_instance_index)
		return false;

	auto offset = m_instance_data_name_to_offset_lut.find(in_name);

	if (offset == m_instance_data_name_to_offset_lut.end())
		return false;

	if (in_texture.is_valid() && in_texture.get_load_state() != Asset_load_state::loaded)
	{
		auto& it = m_texture_parameters_to_apply[(std::to_string(in_instance_index) + "_") + in_name] = in_texture;
		std::string name = in_name;
		it.bind_on_loaded
		(
			[this, name, instance_index = in_instance_index, texture = in_texture](Asset_loaded_processor, Asset*)
			{
				set_parameter_on_instance(name.c_str(), texture, instance_index);
				m_texture_parameters_to_apply.erase((std::to_string(instance_index) + "_") + name);
			}
		);
		return true;
	}

	byte* data_loc = &(m_instance_buffer[in_instance_index + offset->second]);
	GLuint64 handle = in_texture.is_valid() ? in_texture.get()->m_bindless_handle : 0;
	memcpy(data_loc, &handle, sizeof(GLuint64));

	return true;
}

bool sic::Asset_material::set_parameter_on_instance(const char* in_name, const glm::vec4& in_vec4, size_t in_instance_index)
{
	if (m_outermost_parent.is_valid())
	{
		m_outermost_parent.get_mutable()->set_parameter_on_instance(in_name, in_vec4, in_instance_index);
		return true;
	}

	if (m_instance_buffer.size() <= in_instance_index)
		return false;

	auto offset = m_instance_data_name_to_offset_lut.find(in_name);

	if (offset == m_instance_data_name_to_offset_lut.end())
		return false;

	byte* data_loc = &(m_instance_buffer[in_instance_index + offset->second]);
	memcpy(data_loc, &in_vec4, sizeof(glm::vec4));

	return true;
}

void sic::Asset_material::add_default_instance_data()
{
	add_instance_data();

	set_parameters_to_default_on_instance(0);
}

void sic::Asset_material::on_modified()
{
	//refresh parameters

	for (size_t i = 0; i < m_parameters.get_textures().size(); ++i)
	{
		auto& param = m_parameters.get_textures()[i];
		if (!param.m_overriden && m_parent.is_valid())
			param = m_parent.get()->m_parameters.get_textures()[i];
	}

	for (size_t i = 0; i < m_parameters.get_vec4s().size(); ++i)
	{
		auto& param = m_parameters.get_vec4s()[i];
		if (!param.m_overriden && m_parent.is_valid())
			param = m_parent.get()->m_parameters.get_vec4s()[i];
	}

	//refresh instances
	for (size_t idx : m_active_instance_indices)
		set_parameters_to_default_on_instance(idx);

	//notify things who care about modifications
	m_on_modified_delegate.invoke(this);

	this_thread().update_deferred
	(
		[ref = Asset_ref(this)](Engine_context in_context)
		{
			in_context.for_each_w<Component_dynamic_material_parameters>
			(
				[&](Component_dynamic_material_parameters& inout_dynamic_params)
				{
					inout_dynamic_params.reapply_all_parameters(Material_dynamic_parameter_processor(in_context));
				}
			);
		}
	);

	//do again for all children
	for (auto& child : m_children)
		child.get_mutable()->on_modified();
}

size_t sic::Asset_material::add_instance_data_internal(bool in_should_set_parameters)
{
	if (m_outermost_parent.is_valid())
	{
		const size_t ret_val = m_outermost_parent.get_mutable()->add_instance_data_internal(false);
		set_parameters_to_default_on_instance(ret_val);
		return ret_val;
	}

	const ui32 stride = m_instance_vec4_stride * uniform_block_alignment_functions::get_alignment<glm::vec4>();

	if (m_free_instance_buffer_indices.empty())
	{
		const size_t ret_val = m_instance_buffer.size();
		m_instance_buffer.resize(m_instance_buffer.size() + stride);

		if (in_should_set_parameters)
			set_parameters_to_default_on_instance(ret_val);

		return ret_val;
	}

	const size_t free_index = m_free_instance_buffer_indices.back();
	m_free_instance_buffer_indices.pop_back();

	if (in_should_set_parameters)
		set_parameters_to_default_on_instance(free_index);

	return free_index;
}

void sic::Asset_material::remove_instance_data_internal(size_t in_to_remove)
{
	assert(in_to_remove >= 0 && in_to_remove < m_instance_buffer.size() && "Invalid instance data index.");

	m_free_instance_buffer_indices.push_back(in_to_remove);
}

void sic::Asset_material::set_outermost_parent()
{
	Asset_material* outer_parent = m_parent.get_mutable();

	while (outer_parent->m_parent.is_valid())
		outer_parent = outer_parent->m_parent.get_mutable();

	m_outermost_parent = Asset_ref<Asset_material>(outer_parent);
}

void sic::Material_data_layout::add_entry_texture(const char* in_name, Asset_ref<Asset_texture_base> in_default_value)
{
	const std::string name = in_name;

	auto func = [name, in_default_value](Asset_material& inout_material)
	{
		inout_material.set_texture_parameter(name.c_str(), in_default_value);
	};

	const GLuint bytesize = uniform_block_alignment_functions::get_alignment<GLuint64>();
	m_entries.push_back({ func, name, bytesize });

	m_bytesize += bytesize;
}

void sic::Material_data_layout::add_entry_vec4(const char* in_name, const glm::vec4& in_default_value)
{
	const std::string name = in_name;

	auto func = [name, in_default_value](Asset_material& inout_material)
	{
		inout_material.set_vec4_parameter(name.c_str(), in_default_value);
	};

	const GLuint bytesize = uniform_block_alignment_functions::get_alignment<glm::vec4>();
	m_entries.push_back({ func, name, bytesize });

	m_bytesize += bytesize;
}

void sic::Material_data_layout::add_entry_mat4(const char* in_name, const glm::mat4& in_default_value)
{
	const std::string name = in_name;

	auto func = [name, in_default_value](Asset_material& inout_material)
	{
		inout_material.set_mat4_parameter(name.c_str(), in_default_value);
	};

	const GLuint bytesize = uniform_block_alignment_functions::get_alignment<glm::mat4>();
	m_entries.push_back({ func, name, bytesize });

	m_bytesize += bytesize;
}

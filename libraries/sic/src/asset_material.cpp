#pragma once
#include "sic/asset_material.h"
#include "sic/defines.h"
#include "sic/system_asset.h"
#include "sic/magic_enum.h"
#include "sic/logger.h"
#include "sic/shader_parser.h"

#include <unordered_set>

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
		auto& param = m_parameters.m_textures.emplace_back();
		in_stream.read(param.m_name);
		param.m_texture.deserialize(in_assetsystem_state, in_stream);
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

	out_stream.write(m_vertex_shader_code);
	out_stream.write(m_fragment_shader_code);

	out_stream.write(m_is_instanced);
	out_stream.write(m_max_elements_per_drawcall);
	out_stream.write(m_instance_vec4_stride);
	out_stream.write(m_instance_data_name_to_offset_lut);

	out_stream.write(m_parameters.m_textures.size());

	for (const Material_texture_parameter& param : m_parameters.m_textures)
	{
		out_stream.write(param.m_name);
		param.m_texture.serialize(out_stream);
	}
}

bool sic::Asset_material::import(Temporary_string&& in_vertex_shader_path, Temporary_string&& in_fragment_shader_path)
{
	m_vertex_shader_path = in_vertex_shader_path.get_c_str();
	m_fragment_shader_path = in_fragment_shader_path.get_c_str();

	m_vertex_shader_code = Shader_parser::parse_shader(std::move(in_vertex_shader_path)).value_or("");
	m_fragment_shader_code = Shader_parser::parse_shader(std::move(in_fragment_shader_path)).value_or("");

	return true;
}

void sic::Asset_material::get_dependencies(Asset_dependency_gatherer& out_dependencies)
{
	for (auto& tex_param : m_parameters.m_textures)
		out_dependencies.add_dependency(*this, tex_param.m_texture);

	if (m_parent.is_valid())
		out_dependencies.add_dependency(*this, m_parent);

	for (auto& child : m_children)
		if (child.is_valid())
			out_dependencies.add_dependency(*this, child);
}

void sic::Asset_material::enable_instancing(ui32 in_max_elements_per_drawcall, const Material_instance_data_layout& in_data_layout)
{
	m_is_instanced = true;
	m_instance_buffer.reserve((ui64)in_max_elements_per_drawcall * in_data_layout.m_bytesize);
	m_max_elements_per_drawcall = in_max_elements_per_drawcall;
	m_instance_vec4_stride =  std::ceil((float)in_data_layout.m_bytesize / (float)uniform_block_alignment_functions::get_alignment<glm::vec4>());

	ui32 offset = 0;
	for (auto&& entry : in_data_layout.m_entries)
	{
		m_instance_data_name_to_offset_lut[entry.m_name] = offset;
		offset += entry.m_bytesize;
	}
}

void sic::Asset_material::disable_instancing()
{
	m_is_instanced = false;
	m_instance_buffer.clear();
	m_instance_buffer.shrink_to_fit();
}

size_t sic::Asset_material::add_instance_data()
{
	const ui32 stride = m_instance_vec4_stride * uniform_block_alignment_functions::get_alignment<glm::vec4>();

	if (m_free_instance_buffer_indices.empty())
	{
		size_t ret_val = m_instance_buffer.size();
		m_instance_buffer.resize(m_instance_buffer.size() + stride);
		return ret_val;
	}

	const size_t free_index = m_free_instance_buffer_indices.back();
	m_free_instance_buffer_indices.pop_back();

	return free_index;
}

void sic::Asset_material::remove_instance_data(size_t in_to_remove)
{
	assert(in_to_remove >= 0 && in_to_remove < m_instance_buffer.size() && "Invalid instance data index.");

	m_free_instance_buffer_indices.push_back(in_to_remove);
}

void sic::Asset_material::set_parent(Asset_material* in_parent)
{
	if (m_parent.get() == in_parent)
		return;
	
	if (!in_parent && m_parent.is_valid())
		m_parent = Asset_ref<Asset_material>();
	else
		m_parent = in_parent;

	if (m_parent.is_valid())
	{
		m_parameters = m_parent.get()->m_parameters;
		set_outermost_parent();
	}
	else
	{
		m_parameters.m_textures.clear();
	}
}

void sic::Asset_material::set_texture_parameter(const char* in_name, const Asset_ref<Asset_texture>& in_texture)
{
	set_texture_parameter_internal(in_name, in_texture, true);
}

void sic::Asset_material::set_texture_parameter_internal(const char* in_name, const Asset_ref<Asset_texture>& in_texture, bool in_should_override)
{
	if (m_parent.is_valid())
	{
		for (auto& tex_param : m_parameters.m_textures)
		{
			if (tex_param.m_name == in_name)
			{
				if (tex_param.m_overriden && !in_should_override)
					return;

				tex_param.m_texture = in_texture;
				tex_param.m_overriden = true;
			}
		}
	}
	else
	{
		bool was_set = false;
		for (auto& tex_param : m_parameters.m_textures)
		{
			if (tex_param.m_name == in_name)
			{
				tex_param.m_texture = in_texture;
				was_set = true;
			}
		}

		if (!was_set)
			m_parameters.m_textures.push_back({ in_name, in_texture });
	}

	for (auto& child : m_children)
	{
		if (!child.is_valid())
			continue;

		child.get_mutable()->set_texture_parameter_internal(in_name, in_texture, false);
	}
}

void sic::Asset_material::set_outermost_parent()
{
	Asset_material* outer_parent = m_parent.get_mutable();

	while (outer_parent->m_parent.is_valid())
		outer_parent = outer_parent->m_parent.get_mutable();

	m_outermost_parent = outer_parent;
}

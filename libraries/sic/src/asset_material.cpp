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
	in_assetsystem_state;

	in_stream.read(m_vertex_shader_path);
	in_stream.read(m_fragment_shader_path);

	std::string enum_value_name;
	in_stream.read(enum_value_name);
	auto enum_value = magic_enum::enum_cast<Material_blend_mode>(enum_value_name);
	m_blend_mode = enum_value.value_or(Material_blend_mode::Invalid);

	in_stream.read(m_vertex_shader_code);
	in_stream.read(m_fragment_shader_code);

	i32 tex_count = 0;
	in_stream.read(tex_count);

	for (i32 i = 0; i < tex_count; i++)
	{
		auto& param = m_texture_parameters.emplace_back();
		in_stream.read(param.m_name);

		std::string asset_guid;
		in_stream.read(asset_guid);

		if (!asset_guid.empty())
			param.m_texture = in_assetsystem_state.find_asset<Asset_texture>(xg::Guid(asset_guid));
	}
}

void sic::Asset_material::save(const State_assetsystem& in_assetsystem_state, Serialize_stream& out_stream) const
{
	in_assetsystem_state;

	out_stream.write(m_vertex_shader_path);
	out_stream.write(m_fragment_shader_path);

	out_stream.write(magic_enum::enum_name(m_blend_mode));

	out_stream.write(m_vertex_shader_code);
	out_stream.write(m_fragment_shader_code);

	const i32 tex_count = static_cast<ui32>(m_texture_parameters.size());
	out_stream.write(tex_count);

	for (const Texture_parameter& param : m_texture_parameters)
	{
		out_stream.write(param.m_name);
		out_stream.write(param.m_texture.is_valid() ? param.m_texture.get_header()->m_id.str() : std::string(""));
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
	for (auto& tex_param : m_texture_parameters)
		out_dependencies.add_dependency(*this, tex_param.m_texture);
}

void sic::Asset_material::enable_instancing(ui32 in_max_elements_per_drawcall, const char* in_instance_block_name, const Material_instance_data_layout& in_data_layout)
{
	m_is_instanced = true;
	m_instance_buffer.reserve((ui64)in_max_elements_per_drawcall * in_data_layout.m_bytesize);
	m_max_elements_per_drawcall = in_max_elements_per_drawcall;
	m_instance_block_name = in_instance_block_name;
	m_instance_stride = in_data_layout.m_bytesize;

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
	if (m_free_instance_buffer_indices.empty())
	{
		size_t ret_val = m_instance_buffer.size();
		m_instance_buffer.resize(m_instance_buffer.size() + m_instance_stride);
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

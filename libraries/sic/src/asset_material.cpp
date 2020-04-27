#pragma once
#include "sic/asset_material.h"
#include "sic/defines.h"
#include "sic/system_asset.h"
#include "sic/magic_enum.h"
#include "sic/logger.h"
#include "sic/material_parser.h"

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

	m_vertex_shader_code = Material_parser::parse_material(std::move(in_vertex_shader_path)).value_or("Error");
	m_fragment_shader_code = Material_parser::parse_material(std::move(in_fragment_shader_path)).value_or("Error");

	return true;
}

void sic::Asset_material::get_dependencies(Asset_dependency_gatherer& out_dependencies)
{
	for (auto& tex_param : m_texture_parameters)
		out_dependencies.add_dependency(*this, tex_param.m_texture);
}

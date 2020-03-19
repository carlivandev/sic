#pragma once
#include "sic/asset_material.h"
#include "sic/defines.h"
#include "sic/system_asset.h"

void sic::Asset_material::load(const State_assetsystem& in_assetsystem_state, Deserialize_stream& in_stream)
{
	in_assetsystem_state;

	in_stream.read(m_vertex_shader_path);
	in_stream.read(m_fragment_shader_path);

	in_stream.read(m_vertex_shader_code);
	in_stream.read(m_fragment_shader_code);
}

void sic::Asset_material::save(const State_assetsystem& in_assetsystem_state, Serialize_stream& out_stream) const
{
	in_assetsystem_state;

	out_stream.write(m_vertex_shader_path);
	out_stream.write(m_fragment_shader_path);

	out_stream.write(m_vertex_shader_code);
	out_stream.write(m_fragment_shader_code);
}

bool sic::Asset_material::import(const std::string& in_vertex_shader_path, std::string&& in_vertex_shader_code, const std::string& in_fragment_shader_path, std::string&& in_fragment_shader_code)
{
	m_vertex_shader_path = in_vertex_shader_path;
	m_fragment_shader_path = in_fragment_shader_path;

	m_vertex_shader_code = in_vertex_shader_code;
	m_fragment_shader_code = in_fragment_shader_code;

	return true;
}

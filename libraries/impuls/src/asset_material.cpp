#pragma once
#include "impuls/asset_material.h"
#include "impuls/defines.h"
#include "impuls/system_asset.h"

void impuls::Asset_material::load(const State_assetsystem& in_assetsystem_state, Deserialize_stream& in_stream)
{
	in_assetsystem_state;

	in_stream.read(m_vertex_shader);
	in_stream.read(m_fragment_shader);
}

void impuls::Asset_material::save(const State_assetsystem& in_assetsystem_state, Serialize_stream& out_stream) const
{
	in_assetsystem_state;

	out_stream.write(m_vertex_shader);
	out_stream.write(m_fragment_shader);
}

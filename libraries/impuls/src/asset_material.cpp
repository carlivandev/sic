#pragma once
#include "impuls/asset_material.h"
#include "impuls/defines.h"
#include "impuls/system_asset.h"

void impuls::asset_material::load(const state_assetsystem& in_assetsystem_state, deserialize_stream& in_stream)
{
	in_assetsystem_state;

	in_stream.read(m_vertex_shader);
	in_stream.read(m_fragment_shader);
}

void impuls::asset_material::save(const state_assetsystem& in_assetsystem_state, serialize_stream& out_stream) const
{
	in_assetsystem_state;

	out_stream.write(m_vertex_shader);
	out_stream.write(m_fragment_shader);
}

#include "sic/asset_model.h"

#include "sic/system_asset.h"
#include "sic/asset_material.h"

void sic::Asset_model::load(const State_assetsystem& in_assetsystem_state, Deserialize_stream& in_stream)
{
	size_t len = 0;
	in_stream.read(len);

	m_meshes.resize(len);
	m_materials.resize(len);

	for (ui32 i = 0; i < len; i++)
	{
		auto& mesh = m_meshes[i];

		in_stream.read(mesh.m_positions);
		in_stream.read(mesh.m_normals);
		in_stream.read(mesh.m_texcoords);
		in_stream.read(mesh.m_tangents);
		in_stream.read(mesh.m_bitangents);

		in_stream.read(mesh.m_indices);
		in_stream.read(mesh.m_material_slot);

		std::string header_id;
		in_stream.read(header_id);

		if (!header_id.empty())
			m_materials[i] = in_assetsystem_state.find_asset<Asset_material>(xg::Guid(header_id));
	}
}

void sic::Asset_model::save(const State_assetsystem& in_assetsystem_state, Serialize_stream& out_stream) const
{
	in_assetsystem_state;

	out_stream.write(m_meshes.size());

	for (ui32 i = 0; i < m_meshes.size(); i++)
	{
		const auto& mesh = m_meshes[i];
		const auto& mat = m_materials[i];

		out_stream.write(mesh.m_positions);
		out_stream.write(mesh.m_normals);
		out_stream.write(mesh.m_texcoords);
		out_stream.write(mesh.m_tangents);
		out_stream.write(mesh.m_bitangents);

		out_stream.write(mesh.m_indices);
		out_stream.write(mesh.m_material_slot);

		out_stream.write(mat.is_valid() ? mat.get_header()->m_id.str() : std::string(""));
	}
}

void sic::Asset_model::get_dependencies(Asset_dependency_gatherer& out_dependencies)
{
	for (auto& mat_ref : m_materials)
		out_dependencies.add_dependency(*this, mat_ref);
}

const sic::Asset_ref<sic::Asset_material>& sic::Asset_model::get_material(i32 in_index) const
{
	return m_materials[in_index];
}

sic::i32 sic::Asset_model::get_slot_index(const char* in_material_slot) const
{
	const ui64 mesh_count = m_meshes.size();

	for (i32 i = 0; i < mesh_count; i++)
	{
		auto& mesh = m_meshes[i];
		if (mesh.m_material_slot == in_material_slot)
		{
			return i;
		}
	}

	return -1;
}

void sic::Asset_model::set_material(const sic::Asset_ref<sic::Asset_material>& in_material, const char* in_material_slot)
{
	const i32 mesh_idx = get_slot_index(in_material_slot);

	if (mesh_idx == -1)
	{
		SIC_LOG_W(g_log_asset, "Failed to set material, material slot was invalid. Slot name: \"{0}\"", in_material_slot);
		return;
	}

	m_materials[mesh_idx] = in_material;
}

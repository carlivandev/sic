#include "impuls/system_model.h"
#include "impuls/transform.h"
#include "impuls/asset_types.h"

void impuls::system_model::on_created(world_context&& in_context) const
{
	in_context.register_component_type<component_model>("model_component");

	in_context.listen<event_post_created<component_model>>
	(
		[](component_model& in_component)
		{
			in_component.m_transform = in_component.owner().find<component_transform>();
		}
	);
}

void impuls::system_model::on_tick(world_context&& in_context, float in_time_delta) const
{
	in_time_delta;
	in_context;
}

void impuls::component_model::set_material(std::shared_ptr<asset_material>& in_material, const char* in_material_slot)
{
	if (!m_model)
		return;

	const i32 mesh_count = m_model->m_meshes.size();

	for (i32 i = 0; i < mesh_count; i++)
	{
		auto& mesh = m_model->m_meshes[i];
		if (mesh.first.material_slot == in_material_slot)
		{
			if (m_material_overrides.size() <= i)
				m_material_overrides.resize(mesh_count);

			m_material_overrides[i] = in_material;
			return;
		}
	}
}

std::shared_ptr<impuls::asset_material> impuls::component_model::get_material(i32 in_index)
{
	if (!m_model)
		return nullptr;

	if (in_index < m_material_overrides.size() && m_material_overrides[in_index])
	{
		return m_material_overrides[in_index];
	}

	return m_model->get_material(in_index);
}

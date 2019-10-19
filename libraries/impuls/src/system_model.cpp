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

	const i32 mesh_idx = m_model->get_slot_index(in_material_slot);

	if (mesh_idx == -1)
		return;

	if (m_material_overrides.size() <= mesh_idx)
		m_material_overrides.resize((size_t)mesh_idx + 1);

	m_material_overrides[mesh_idx] = in_material;
}

std::shared_ptr<impuls::asset_material> impuls::component_model::get_material(i32 in_index) const
{
	if (!m_model)
		return nullptr;

	if (in_index < m_material_overrides.size() && m_material_overrides[in_index])
	{
		return m_material_overrides[in_index];
	}

	return m_model->get_material(in_index);
}

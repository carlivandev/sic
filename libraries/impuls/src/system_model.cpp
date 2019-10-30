#include "impuls/system_model.h"
#include "impuls/transform.h"
#include "impuls/asset_types.h"
#include "impuls/system_renderer.h"

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

	state_renderer* renderer_state = in_context.get_state<state_renderer>();

	if (!renderer_state)
		return;

	renderer_state->m_model_drawcalls.write
	(
		[&in_context](std::vector<drawcall_model>& out_drawcalls)
		{
			for (component_model& model : in_context.each<component_model>())
			{
				if (!model.m_model.is_valid())
					continue;

				out_drawcalls.push_back(drawcall_model{ model.m_model, model.m_material_overrides, model.m_transform->m_position });
			}
		}
	);
	
}

void impuls::component_model::set_material(asset_ref<asset_material> in_material, const char* in_material_slot)
{
	m_material_overrides[in_material_slot] = in_material;
}

impuls::asset_ref<impuls::asset_material> impuls::component_model::get_material_override(const char* in_material_slot) const
{
	auto it = m_material_overrides.find(in_material_slot);

	if (it == m_material_overrides.end())
		return asset_ref <asset_material>();

	return it->second;
}

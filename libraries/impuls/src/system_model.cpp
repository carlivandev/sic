#include "impuls/system_model.h"

#include "impuls/transform.h"
#include "impuls/asset_types.h"
#include "impuls/system_renderer.h"
#include "impuls/state_render_scene.h"

void impuls::system_model::on_created(world_context&& in_context) const
{
	in_context.register_component_type<component_model>("model_component");

	in_context.listen<event_post_created<component_model>>
	(
		[](world_context& in_out_context, component_model& in_out_component)
		{
			in_out_component.m_transform = in_out_component.owner().find<component_transform>();
			
			in_out_component.m_render_object_id = in_out_context.get_state<state_render_scene>()->m_models.create_render_object
			(
				[
					model = in_out_component.m_model,
					material_overrides = in_out_component.m_material_overrides,
					position = in_out_component.m_transform->m_position
				](render_object_model& in_out_object)
				{
					in_out_object.m_model = model;
					in_out_object.m_material_overrides = material_overrides;
					in_out_object.m_position = position;
				}
			);
		}
	);

	in_context.listen<event_destroyed<component_model>>
	(
		[](world_context& in_out_context, component_model& in_out_component)
		{
			in_out_context.get_state<state_render_scene>()->m_models.destroy_render_object(in_out_component.m_render_object_id);
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
			for (component_model& model : in_context.components<component_model>())
			{
				if (!model.m_model.is_valid())
					continue;

				out_drawcalls.push_back(drawcall_model{ model.m_model, model.m_material_overrides, model.m_transform->m_position });
			}
		}
	);
}

void impuls::component_model::set_material(world_context in_context, asset_ref<asset_material> in_material, const std::string& in_material_slot)
{
	m_material_overrides[in_material_slot] = in_material;

	in_context.get_state<state_render_scene>()->m_models.update_render_object
	(
		m_render_object_id,
		[in_material_slot, in_material](render_object_model& in_object)
		{
			in_object.m_material_overrides[in_material_slot] = in_material;
		} 
	);
}

impuls::asset_ref<impuls::asset_material> impuls::component_model::get_material_override(const std::string& in_material_slot) const
{
	auto it = m_material_overrides.find(in_material_slot);

	if (it == m_material_overrides.end())
		return asset_ref <asset_material>();

	return it->second;
}

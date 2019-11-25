#include "impuls/system_model.h"

#include "impuls/component_transform.h"
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
			component_transform* transform = in_out_component.owner().find<component_transform>();
			assert(transform && "model component requires a transform attached!");

			in_out_component.m_render_scene_state = in_out_context.get_state<state_render_scene>();

			in_out_component.m_on_position_changed_handle.m_function =
			[&in_out_component](const glm::vec3& in_pos)
			{
				in_out_component.m_render_scene_state->m_models.update_render_object
				(
					in_out_component.m_render_object_id,
					[in_pos](render_object_model& in_object)
					{
						in_object.m_position = in_pos;
					}
				);
			};

			transform->m_on_position_changed.bind(in_out_component.m_on_position_changed_handle);

			in_out_component.m_render_object_id = in_out_component.m_render_scene_state->m_models.create_render_object
			(
				[
					model = in_out_component.m_model,
					material_overrides = in_out_component.m_material_overrides,
					position = transform->get_position()
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
		[](world_context&, component_model& in_out_component)
		{
			in_out_component.m_render_scene_state->m_models.destroy_render_object(in_out_component.m_render_object_id);
		}
	);
}

void impuls::component_model::set_model(const asset_ref<asset_model>& in_model)
{
	m_model = in_model;

	m_render_scene_state->m_models.update_render_object
	(
		m_render_object_id,
		[in_model](render_object_model& in_object)
		{
			in_object.m_model = in_model;
		} 
	);
}

void impuls::component_model::set_material(asset_ref<asset_material> in_material, const std::string& in_material_slot)
{
	m_material_overrides[in_material_slot] = in_material;

	m_render_scene_state->m_models.update_render_object
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

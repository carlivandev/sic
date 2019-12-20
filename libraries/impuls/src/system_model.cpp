#include "impuls/system_model.h"

#include "impuls/component_transform.h"
#include "impuls/asset_types.h"
#include "impuls/system_renderer.h"
#include "impuls/state_render_scene.h"

void impuls::System_model::on_created(Engine_context&& in_context)
{
	in_context.register_component_type<Component_model>("model_component");

	in_context.listen<event_post_created<Component_model>>
	(
		[](Engine_context& in_out_context, Component_model& in_out_component)
		{
			Component_transform* transform = in_out_component.owner().find<Component_transform>();
			assert(transform && "model component requires a Transform attached!");

			in_out_component.m_render_scene_state = in_out_context.get_state<State_render_scene>();

			in_out_component.m_on_updated_handle.m_function =
			[&in_out_component](const Component_transform& in_transform)
			{
				in_out_component.m_render_scene_state->m_models.update_object
				(
					in_out_component.m_render_object_id,
					[pos = in_transform.get_translation()](Render_object_model& in_object)
					{
						in_object.m_position = pos;
					}
				);
			};

			transform->m_on_updated.bind(in_out_component.m_on_updated_handle);

			in_out_component.m_render_object_id = in_out_component.m_render_scene_state->m_models.create_object
			(
				[
					model = in_out_component.m_model,
					material_overrides = in_out_component.m_material_overrides,
					position = transform->get_translation()
				](Render_object_model& in_out_object)
				{
					in_out_object.m_model = model;
					in_out_object.m_material_overrides = material_overrides;
					in_out_object.m_position = position;
				}
			);
		}
	);

	in_context.listen<event_destroyed<Component_model>>
	(
		[](Engine_context&, Component_model& in_out_component)
		{
			in_out_component.m_render_scene_state->m_models.destroy_object(in_out_component.m_render_object_id);
		}
	);
}

void impuls::Component_model::set_model(const Asset_ref<Asset_model>& in_model)
{
	m_model = in_model;

	m_render_scene_state->m_models.update_object
	(
		m_render_object_id,
		[in_model](Render_object_model& in_object)
		{
			in_object.m_model = in_model;
		} 
	);
}

void impuls::Component_model::set_material(Asset_ref<Asset_material> in_material, const std::string& in_material_slot)
{
	m_material_overrides[in_material_slot] = in_material;

	m_render_scene_state->m_models.update_object
	(
		m_render_object_id,
		[in_material_slot, in_material](Render_object_model& in_object)
		{
			in_object.m_material_overrides[in_material_slot] = in_material;
		} 
	);
}

impuls::Asset_ref<impuls::Asset_material> impuls::Component_model::get_material_override(const std::string& in_material_slot) const
{
	auto it = m_material_overrides.find(in_material_slot);

	if (it == m_material_overrides.end())
		return Asset_ref <Asset_material>();

	return it->second;
}

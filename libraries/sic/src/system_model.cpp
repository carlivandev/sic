#include "sic/system_model.h"

#include "sic/component_transform.h"
#include "sic/asset_types.h"
#include "sic/system_renderer.h"
#include "sic/state_render_scene.h"

void sic::System_model::on_created(Engine_context in_context)
{
	in_context.register_component_type<Component_model>("model_component");

	in_context.listen<event_post_created<Component_model>>
	(
		[](Engine_context& in_out_context, Component_model& in_out_component)
		{
			Component_transform* transform = in_out_component.get_owner().find<Component_transform>();
			assert(transform && "model component requires a Transform attached!");

			in_out_component.m_render_scene_state = in_out_context.get_state<State_render_scene>();

			in_out_component.m_on_updated_handle.set_callback
			(
				[&in_out_component](const Component_transform& in_transform)
				{
					if (!in_out_component.m_render_object_id.is_valid())
						return;

					in_out_component.m_render_scene_state->update_object
					(
						in_out_component.m_render_object_id,
						[
							orientation = in_transform.get_matrix()
						](Render_object_model& in_object)
						{
							in_object.m_orientation = orientation;
						}
					);
				}
			);

			transform->m_on_updated.bind(in_out_component.m_on_updated_handle);
			in_out_component.try_create_render_object();
		}
	);

	in_context.listen<event_destroyed<Component_model>>
	(
		[](Engine_context&, Component_model& in_out_component)
		{
			in_out_component.m_render_scene_state->destroy_object(in_out_component.m_render_object_id);
		}
	);
}

void sic::Component_model::set_model(const Asset_ref<Asset_model>& in_model)
{
	if (m_model == in_model)
		return;

	m_model = in_model;

	if (m_model.get_load_state() == Asset_load_state::loaded && m_render_object_id.is_valid())
	{
		m_render_scene_state->update_object
		(
			m_render_object_id,
			[in_model](Render_object_model& in_object)
			{
				in_object.m_model = in_model;
			} 
		);
	}
	else
	{
		try_destroy_render_object();
		m_model.bind_on_loaded([this](Asset*) { try_create_render_object(); });
	}
}

void sic::Component_model::set_material(Asset_ref<Asset_material> in_material, const std::string& in_material_slot)
{
	Asset_ref<Asset_material>& override_mat = m_material_overrides[in_material_slot];

	if (override_mat == in_material)
		return;

	override_mat = in_material;

	if (!m_render_object_id.is_valid())
		return;

	const bool invalid_or_loaded = !override_mat.is_valid() || override_mat.get_load_state() == Asset_load_state::loaded;
	const bool valid_and_not_loaded = override_mat.is_valid() && override_mat.get_load_state() != Asset_load_state::loaded;

	if (invalid_or_loaded)
	{
		m_render_scene_state->update_object
		(
			m_render_object_id,
			[in_material_slot, override_mat](Render_object_model& in_object)
			{
				in_object.m_material_overrides[in_material_slot] = override_mat;
			} 
		);
	}
	else if (valid_and_not_loaded)
	{
		override_mat.bind_on_loaded
		(
			[this, in_material_slot, override_mat](Asset*)
			{
				if (!m_render_object_id.is_valid())
					return;

				m_render_scene_state->update_object
				(
					m_render_object_id,
					[in_material_slot, override_mat](Render_object_model& in_object)
					{
						in_object.m_material_overrides[in_material_slot] = override_mat;
					}
				);
			}
		);
	}
}

sic::Asset_ref<sic::Asset_material> sic::Component_model::get_material_override(const std::string& in_material_slot) const
{
	auto it = m_material_overrides.find(in_material_slot);

	if (it == m_material_overrides.end())
		return Asset_ref<Asset_material>();

	return it->second;
}

void sic::Component_model::try_destroy_render_object()
{
	if (m_render_object_id.is_valid())
	{
		m_render_scene_state->destroy_object(m_render_object_id);
		m_render_object_id.reset();
	}
}

void sic::Component_model::try_create_render_object()
{
	if (!m_model.is_valid())
		return;

	if (m_model.get_load_state() != Asset_load_state::loaded)
		return;

	//for now just skip creating, but in the future we want to send an error material to the renderer if it has an override that is invalid
	for (auto&& override_it : m_material_overrides)
	{
		const Asset_ref<Asset_material>& mat = override_it.second;

		if (!mat.is_valid())
			return;

		if (mat.get_load_state() != Asset_load_state::loaded)
			return;
	}

	m_render_object_id = m_render_scene_state->create_object<Render_object_model>
	(
		get_owner().get_outermost_level_id(),
		[
			model = m_model,
			material_overrides = m_material_overrides,
			orientation = get_owner().find<Component_transform>()->get_matrix()
		]
		(Render_object_model& in_out_object)
		{
			in_out_object.m_model = model;
			in_out_object.m_material_overrides = material_overrides;
			in_out_object.m_orientation = orientation;
		}
	);
}
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
					if (in_out_component.m_render_object_id_collection.empty())
						return;

					for (const auto& render_object_id : in_out_component.m_render_object_id_collection)
					{
						in_out_component.m_render_scene_state->update_object
						(
							render_object_id,
							[
								orientation = in_transform.get_matrix()
							](Render_object_mesh& in_object)
							{
								in_object.m_orientation = orientation;
							}
						);
					}
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
			in_out_component.try_destroy_render_object();
		}
	);
}

void sic::Component_model::set_model(const Asset_ref<Asset_model>& in_model)
{
	if (m_model == in_model)
		return;

	m_model = in_model;

	if (m_model.get_load_state() == Asset_load_state::loaded && !m_render_object_id_collection.empty())
	{
		for (size_t idx = 0; idx < m_render_object_id_collection.size(); idx++)
		{
			auto& mesh = m_model.get()->m_meshes[idx];

			auto material_override_it = m_material_overrides.find(mesh.m_material_slot);

			const Asset_ref<Asset_material> mat_to_draw = material_override_it != m_material_overrides.end() ? material_override_it->second : m_model.get()->get_material((i32)idx);

			m_render_scene_state->update_object
			(
				m_render_object_id_collection[idx],
				[model = m_model, mat_to_draw, idx](Render_object_mesh& in_object)
				{
					in_object.m_mesh = &(model.get()->m_meshes[idx]);
					in_object.m_material->remove_instance_data(in_object.m_instance_data_index);

					in_object.m_material = mat_to_draw.get_mutable();
					in_object.m_instance_data_index = in_object.m_material->add_instance_data();
				} 
			);
		}
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

	if (m_render_object_id_collection.empty())
		return;

	const bool invalid_or_loaded = !override_mat.is_valid() || override_mat.get_load_state() == Asset_load_state::loaded;
	const bool valid_and_not_loaded = override_mat.is_valid() && override_mat.get_load_state() != Asset_load_state::loaded;

	if (invalid_or_loaded)
	{
		i32 idx = m_model.get()->get_slot_index(in_material_slot.c_str());

		if (idx != -1)
		{
			auto& render_object_id = m_render_object_id_collection[idx];

			m_render_scene_state->update_object
			(
				render_object_id,
				[in_material_slot, override_mat](Render_object_mesh& in_object)
				{
					in_object.m_material->remove_instance_data(in_object.m_instance_data_index);

					in_object.m_material = override_mat.get_mutable();
					in_object.m_instance_data_index = in_object.m_material->add_instance_data();
				} 
			);
		}
	}
	else if (valid_and_not_loaded)
	{
		override_mat.bind_on_loaded
		(
			[this, in_material_slot, override_mat](Asset*)
			{
				if (m_render_object_id_collection.empty())
					return;

				i32 idx = m_model.get()->get_slot_index(in_material_slot.c_str());

				if (idx != -1)
				{
					auto& render_object_id = m_render_object_id_collection[idx];

					m_render_scene_state->update_object
					(
						render_object_id,
						[in_material_slot, override_mat](Render_object_mesh& in_object)
						{
							in_object.m_material->remove_instance_data(in_object.m_instance_data_index);

							in_object.m_material = override_mat.get_mutable();
							in_object.m_instance_data_index = in_object.m_material->add_instance_data();
						}
					);
				}
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
	for (size_t i = 0; i < m_render_object_id_collection.size(); i++)
	{
		auto& render_object_id = m_render_object_id_collection[i];
		if (render_object_id.is_valid())
		{
			auto material_override_it = m_material_overrides.find(m_model.get()->m_meshes[i].m_material_slot);
			const Asset_ref<Asset_material> mat_to_draw = material_override_it != m_material_overrides.end() ? material_override_it->second : m_model.get()->get_material((i32)i);

			m_render_scene_state->update_object<Render_object_mesh>
			(
				render_object_id,
				[model = m_model, mat_to_draw  /*capture it by copy to keep asset loaded until destroyed is called*/] (Render_object_mesh& in_out_object)
				{
					in_out_object.m_material->remove_instance_data(in_out_object.m_instance_data_index);
				}
			);
			m_render_scene_state->destroy_object(render_object_id);
			render_object_id.reset();
		}
	}

	m_render_object_id_collection.clear();
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

	for (ui32 mesh_idx = 0; mesh_idx < m_model.get()->m_meshes.size(); mesh_idx++)
	{
		auto& mesh = m_model.get()->m_meshes[mesh_idx];

		auto material_override_it = m_material_overrides.find(mesh.m_material_slot);
		const Asset_ref<Asset_material> mat_to_draw = material_override_it != m_material_overrides.end() ? material_override_it->second : m_model.get()->get_material(mesh_idx);

		m_render_object_id_collection.emplace_back
		(
			m_render_scene_state->create_object<Render_object_mesh>
			(
				get_owner().get_outermost_level_id(),
				[
					model = m_model,
					mat_to_draw,
					mesh_idx,
					orientation = get_owner().find<Component_transform>()->get_matrix()
				]
				(Render_object_mesh& in_out_object)
				{
					in_out_object.m_mesh = &(model.get()->m_meshes[mesh_idx]);
					in_out_object.m_material = mat_to_draw.get_mutable();
					in_out_object.m_orientation = orientation;
					in_out_object.m_instance_data_index = in_out_object.m_material->add_instance_data();
				}
			)
		);
	}
}
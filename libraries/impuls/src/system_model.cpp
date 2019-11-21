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
	//TODO: finish this
	struct render_object
	{
		asset_ref<asset_model> m_model;
		std::unordered_map<std::string, asset_ref<asset_material>> m_material_overrides;
		glm::vec3 m_position = { 0.0f, 0.0f, 0.0f };
	};

	struct render_object_update
	{
		using callback = std::function<void(render_object& in_out_object)>;

		callback m_callback;
		i32 m_render_object_id = -1;
	};

	struct state_render_scene : i_state
	{
		i32 request_render_object()
		{
			const i32 id = static_cast<i32>(m_id_to_render_object_index_lut.size());

			//TODO: should use a thread-safe ticker that gets set in the sync step, and not touch render_objects directly since size can change on render thread
			const size_t render_object_index = m_render_objects.size();

			m_render_objects_to_create.write
			(
				[id, render_object_index](std::vector<std::pair<i32, i32>>& in_out_to_creates)
				{
					in_out_to_creates.push_back({ id, render_object_index });
				}
			);

			return id;
		}

		void create_render_objects()
		{
			m_render_objects_to_create.read
			(
				[this](const std::vector<std::pair<i32, i32>>& in_to_creates)
				{
					for (auto&& id_to_render_object_idx_pair : in_to_creates)
					{

					}
				}
			);
		}

		void request_update(i32 in_object_id, render_object_update::callback&& in_update_callback)
		{
			m_render_object_updates.write
			(
				[&in_object_id, &in_update_callback](std::vector<render_object_update>& in_out_updates)
				{
					in_out_updates.push_back({ in_update_callback, in_object_id });
				}
			);
		}

	protected:
		//TODO: all of this should be in a templated class so we can do the exact same behaviour for all kinds of render_objects
		//render_object_list<render_object_mesh> etc

		std::vector<render_object> m_render_objects;
		std::vector<i32> m_id_to_render_object_index_lut;
		std::vector<i32> m_render_object_index_to_id_lut;

		double_buffer<std::vector<std::pair<i32, i32>>> m_render_objects_to_create;
		double_buffer<std::vector<render_object_update>> m_render_object_updates;
		//TODO: request_update for render_material and render_light and whatever else needs it
	};

	m_material_overrides[in_material_slot] = in_material;

	in_context.get_state<state_render_scene>()->request_update
	(
		m_render_object_id,
		[in_material_slot, in_material](render_object& in_object)
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

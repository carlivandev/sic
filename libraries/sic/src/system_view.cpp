#include "sic/system_view.h"

#include "sic/state_render_scene.h"

void sic::System_view::on_created(Engine_context&& in_context)
{
	in_context.register_component_type<Component_view>("component_view", 4);
	in_context.register_object<Object_view>("view", 4);

	in_context.listen<event_post_created<Component_view>>
	(
		[](Engine_context& in_out_context, Component_view& in_out_component)
		{
			Component_transform* transform = in_out_component.owner().find<Component_transform>();
			assert(transform && "view component requires a Transform attached!");

			in_out_component.m_render_scene_state = in_out_context.get_state<State_render_scene>();

			in_out_component.m_on_updated_handle.m_function =
			[&in_out_component](const Component_transform& in_transform)
			{
				in_out_component.m_render_scene_state->m_views.update_object
				(
					in_out_component.m_render_object_id,
					[matrix = in_transform.get_matrix()]
					(Render_object_view& in_object)
					{
						in_object.m_view_orientation = matrix;
					}
				);
			};
			
			transform->m_on_updated.bind(in_out_component.m_on_updated_handle);

			in_out_component.m_render_object_id = in_out_component.m_render_scene_state->m_views.create_object
			(
				[
					matrix = transform->get_matrix(),
					viewport_offset = in_out_component.m_viewport_offset,
					viewport_size = in_out_component.m_viewport_size
				]
				(Render_object_view& in_object)
				{
					in_object.m_view_orientation = matrix;
					in_object.m_viewport_offset = viewport_offset;
					in_object.m_viewport_size = viewport_size;
				}
			);
		}
	);

	in_context.listen<event_destroyed<Component_view>>
	(
		[](Engine_context&, Component_view& in_out_component)
		{
			in_out_component.m_render_scene_state->m_views.destroy_object(in_out_component.m_render_object_id);
		}
	);
}
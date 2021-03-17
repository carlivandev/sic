#include "sic/system_view.h"

#include "sic/state_render_scene.h"

void sic::System_view::on_created(Engine_context in_context)
{
	in_context.register_component<Component_view>("component_view", 4);
	in_context.register_object<Object_view>("view", 4);

	in_context.listen<Event_post_created<Component_view>>
	(
		[](Engine_context& in_out_context, Component_view& in_out_component)
		{
			Component_transform* transform = in_out_context.find_w<Component_transform>(in_out_component.get_owner());
			assert(transform && "view component requires a Component_transform attached!");

			in_out_component.m_render_object_id.set(in_out_component.get_owner().get_outermost_scene_id(), in_out_component.get_owner().get_id().get_id());

			in_out_component.m_on_updated_handle.set_callback
			(
				[in_out_context, ro_id = in_out_component.m_render_object_id](const Component_transform& in_transform) mutable
				{
					in_out_context.schedule(std::function([matrix = in_transform.get_matrix(), ro_id]
					(Engine_processor<Processor_flag_write<State_render_scene>> in_processor)
					{
						in_processor.get_state_checked_w<State_render_scene>().update_object
						(
							ro_id,
							[matrix] (Render_object_view& in_object)
							{
								in_object.m_view_orientation = matrix;
							}
						);
					}));
				}
			);
			
			transform->m_on_updated.try_bind(in_out_component.m_on_updated_handle);

			in_out_context.get_state_checked<State_render_scene>().create_object<Render_object_view>
			(
				in_out_component.m_render_object_id,
				[
					level_id = in_out_component.m_render_object_id.m_level_id,
					matrix = transform->get_matrix(),
					viewport_offset = in_out_component.m_viewport_offset,
					viewport_size = in_out_component.m_viewport_size,
					viewport_dimensions = in_out_component.m_viewport_dimensions,
					near_plane = in_out_component.m_near_plane,
					far_plane = in_out_component.m_far_plane,
					render_target = in_out_component.m_render_target
				]
				(Render_object_view& in_object)
				{
					in_object.m_level_id = level_id;
					in_object.m_view_orientation = matrix;
					in_object.m_viewport_offset = viewport_offset;
					in_object.m_viewport_size = viewport_size;
					in_object.m_near_plane = near_plane;
					in_object.m_far_plane = far_plane;

					in_object.m_render_target = render_target;
				}
			);

		}
	);

	in_context.listen<Event_destroyed<Component_view>>
	(
		[](Engine_context& inout_context, Component_view& in_out_component)
		{
			inout_context.schedule
			(
				std::function([ro_id = in_out_component.m_render_object_id](Engine_processor<Processor_flag_write<State_render_scene>> in_processor)
				{
					in_processor.get_state_checked_w<State_render_scene>().destroy_object(ro_id);
				})
			);
		}
	);
}
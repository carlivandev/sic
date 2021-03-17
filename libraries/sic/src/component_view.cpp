#include "sic/component_view.h"

#include "sic/state_render_scene.h"
#include "sic/system_window.h"

void sic::Component_view::set_window(Engine_processor<Processor_flag_write<State_render_scene>> in_processor, Window_proxy* in_window)
{
	if (m_window)
		m_window->m_on_destroyed.unbind(m_on_window_destroyed_handle);

	m_window = in_window;

	if (m_window)
	{
		m_on_window_destroyed_handle.set_callback
		(
			[ro_id = m_render_object_id](Processor_window in_processor)
			{
				in_processor.get_state_checked_w<State_render_scene>().update_object
				(
					ro_id,
					[](Render_object_view& inout_view)
					{
						inout_view.m_window_id.reset();
						inout_view.m_invalidated = true;
					}
				);
			}
		);

		m_window->m_on_destroyed.try_bind(m_on_window_destroyed_handle);
	}

	in_processor.get_state_checked_w<State_render_scene>().update_object
	(
		m_render_object_id,
		[window = in_window](Render_object_view& inout_view)
		{
			if (window)
				inout_view.m_window_id = window->m_window_id;
			else
				inout_view.m_window_id.reset();

			inout_view.m_invalidated = true;
		}
	);
}

void sic::Component_view::set_render_target(Engine_processor<Processor_flag_write<State_render_scene>> in_processor, Asset_ref<Asset_render_target> in_render_target)
{
	m_render_target = in_render_target;

	in_processor.get_state_checked_w<State_render_scene>().update_object
	(
		m_render_object_id,
		[in_render_target](Render_object_view& inout_view)
		{
			inout_view.m_render_target = in_render_target;
			inout_view.m_invalidated = true;
		}
	);
}

void sic::Component_view::set_viewport_offset(Engine_processor<Processor_flag_write<State_render_scene>> in_processor, const glm::vec2& in_viewport_offset)
{
	m_viewport_offset = in_viewport_offset;

	in_processor.get_state_checked_w<State_render_scene>().update_object
	(
		m_render_object_id,
		[in_viewport_offset](Render_object_view& inout_view)
		{
			inout_view.m_viewport_offset = in_viewport_offset;
			inout_view.m_invalidated = true;
		}
	);
}

void sic::Component_view::set_viewport_size(Engine_processor<Processor_flag_write<State_render_scene>> in_processor, const glm::vec2& in_viewport_size)
{
	m_viewport_size = in_viewport_size;

	in_processor.get_state_checked_w<State_render_scene>().update_object
	(
		m_render_object_id,
		[in_viewport_size](Render_object_view& inout_view)
		{
			inout_view.m_viewport_size = in_viewport_size;
			inout_view.m_invalidated = true;
		}
	);
}

void sic::Component_view::set_fov(Engine_processor<Processor_flag_write<State_render_scene>> in_processor, float in_fov)
{
	m_fov = in_fov;

	in_processor.get_state_checked_w<State_render_scene>().update_object
	(
		m_render_object_id,
		[in_fov](Render_object_view& inout_view)
		{
			inout_view.m_fov = in_fov;
			inout_view.m_invalidated = true;
		}
	);
}

void sic::Component_view::set_near_and_far_plane(Engine_processor<Processor_flag_write<State_render_scene>> in_processor, float in_near, float in_far)
{
	m_near_plane = in_near;
	m_far_plane = in_far;

	in_processor.get_state_checked_w<State_render_scene>().update_object
	(
		m_render_object_id,
		[in_near, in_far](Render_object_view& inout_view)
		{
			inout_view.m_near_plane = in_near;
			inout_view.m_far_plane = in_far;
			inout_view.m_invalidated = true;
		}
	);
}

void sic::Component_view::set_realtime(Engine_processor<Processor_flag_write<State_render_scene>> in_processor, bool in_realtime)
{
	m_realtime = in_realtime;

	in_processor.get_state_checked_w<State_render_scene>().update_object
	(
		m_render_object_id,
		[in_realtime](Render_object_view& inout_view)
		{
			inout_view.m_realtime = in_realtime;
			inout_view.m_invalidated = true;
		}
	);
}

void sic::Component_view::invalidate(Engine_processor<Processor_flag_write<State_render_scene>> in_processor)
{
	in_processor.get_state_checked_w<State_render_scene>().update_object
	(
		m_render_object_id,
		[](Render_object_view& inout_view)
		{
			inout_view.m_invalidated = true;
		}
	);
}

glm::mat4 sic::Component_view::calculate_projection_matrix() const
{
	if (!m_window)
		return glm::mat4(1);

	const glm::ivec2 window_dimensions = m_window->get_dimensions();

	if (window_dimensions.x == 0 || window_dimensions.y == 0)
		return glm::mat4(1);

	const float view_aspect_ratio = (window_dimensions.x * m_viewport_size.x) / (window_dimensions.y * m_viewport_size.y);

	const glm::mat4x4 proj_mat = glm::perspective
	(
		glm::radians(m_fov),
		view_aspect_ratio,
		m_near_plane,
		m_far_plane
	);

	return proj_mat;
}
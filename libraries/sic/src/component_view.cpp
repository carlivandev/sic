#include "sic/component_view.h"

#include "sic/state_render_scene.h"
#include "sic/system_window.h"

void sic::Component_view::set_window(Object_window* in_window)
{
	m_window_render_on = in_window;

	Component_window& cw = in_window->get<Component_window>();

	if (cw.m_window)
	{
		m_render_scene_state->m_views.update_object
		(
			m_render_object_id,
			[window = cw.m_window](Render_object_view& inout_view)
			{
				inout_view.m_window_render_on = window;
			}
		);
	}
	else
	{
		cw.m_on_window_created.bind(m_on_window_created_handle);
		m_on_window_created_handle.m_function =
		[this](GLFWwindow* window)
		{
			m_render_scene_state->m_views.update_object
			(
				m_render_object_id,
				[window](Render_object_view& inout_view)
				{
					inout_view.m_window_render_on = window;
				}
			);
		};
	}
}

void sic::Component_view::set_viewport_offset(const glm::vec2& in_viewport_offset)
{
	m_viewport_offset = in_viewport_offset;

	m_render_scene_state->m_views.update_object
	(
		m_render_object_id,
		[in_viewport_offset](Render_object_view& inout_view)
		{
			inout_view.m_viewport_offset = in_viewport_offset;
		}
	);
}

void sic::Component_view::set_viewport_size(const glm::vec2& in_viewport_size)
{
	m_viewport_size = in_viewport_size;

	m_render_scene_state->m_views.update_object
	(
		m_render_object_id,
		[in_viewport_size](Render_object_view& inout_view)
		{
			inout_view.m_viewport_size = in_viewport_size;
		}
	);
}

void sic::Component_view::set_fov(float in_fov)
{
	m_fov = in_fov;

	m_render_scene_state->m_views.update_object
	(
		m_render_object_id,
		[in_fov](Render_object_view& inout_view)
		{
			inout_view.m_fov = in_fov;
		}
	);
}

void sic::Component_view::set_near_and_far_plane(float in_near, float in_far)
{
	m_near_plane = in_near;
	m_far_plane = in_far;

	m_render_scene_state->m_views.update_object
	(
		m_render_object_id,
		[in_near, in_far](Render_object_view& inout_view)
		{
			inout_view.m_near_plane = in_near;
			inout_view.m_far_plane = in_far;
		}
	);
}

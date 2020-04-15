#include "sic/component_view.h"

#include "sic/state_render_scene.h"
#include "sic/system_window.h"

void sic::Component_view::set_window(Window_interface* in_window)
{
	if (m_window)
		m_window->m_on_destroyed.unbind(m_on_window_destroyed_handle);

	m_window = in_window;

	if (m_window)
	{
		m_on_window_destroyed_handle.m_function = [this, in_window]()
		{
			m_render_scene_state->update_object
			(
				m_render_object_id,
				[](Render_object_view& inout_view)
				{
					inout_view.m_window_id.reset();
				}
			);
		};

		m_window->m_on_destroyed.bind(m_on_window_destroyed_handle);
	}

	m_render_scene_state->update_object
	(
		m_render_object_id,
		[window = in_window](Render_object_view& inout_view)
		{
			if (window)
				inout_view.m_window_id = window->m_window_id;
			else
				inout_view.m_window_id.reset();
		}
	);
}

void sic::Component_view::set_viewport_dimensions(const glm::ivec2& in_viewport_dimensions)
{
	m_viewport_dimensions = in_viewport_dimensions;

	m_render_scene_state->update_object
	(
		m_render_object_id,
		[in_viewport_dimensions](Render_object_view& inout_view)
		{
			inout_view.m_render_target.reset();
			inout_view.m_render_target.emplace(in_viewport_dimensions, true);
		}
	);
}

void sic::Component_view::set_viewport_offset(const glm::vec2& in_viewport_offset)
{
	m_viewport_offset = in_viewport_offset;

	m_render_scene_state->update_object
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

	m_render_scene_state->update_object
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

	m_render_scene_state->update_object
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

	m_render_scene_state->update_object
	(
		m_render_object_id,
		[in_near, in_far](Render_object_view& inout_view)
		{
			inout_view.m_near_plane = in_near;
			inout_view.m_far_plane = in_far;
		}
	);
}

void sic::Component_view::set_clear_color(const glm::vec4& in_clear_color)
{
	m_clear_color = in_clear_color;

	m_render_scene_state->update_object
	(
		m_render_object_id,
		[in_clear_color](Render_object_view& inout_view)
		{
			inout_view.m_render_target.value().set_clear_color(in_clear_color);
		}
	);
}
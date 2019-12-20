#include "impuls/component_view.h"

#include "impuls/state_render_scene.h"
#include "impuls/system_window.h"

void impuls::Component_view::set_window(Object_window* in_window)
{
	m_window_render_on = in_window;

	m_render_scene_state->m_views.update_object
	(
		m_render_object_id,
		[window = in_window->get<Component_window>().m_window](Render_object_view& inout_view)
		{
			inout_view.m_window_render_on = window;
		}
	);
}

void impuls::Component_view::set_viewport_offset(const glm::vec2& in_viewport_offset)
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

void impuls::Component_view::set_viewport_size(const glm::vec2& in_viewport_size)
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

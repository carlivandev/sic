#pragma once
#include "impuls/component.h"

#include "impuls/component_transform.h"
#include "impuls/gl_includes.h"
#include "impuls/object.h"
#include "impuls/state_render_scene.h"

#include "glm/vec3.hpp"

namespace impuls
{
	struct state_render_scene;
	struct object_window;

	struct component_view : public i_component
	{
		friend struct system_view;

		void set_window(object_window* in_window);
		void set_viewport_offset(const glm::vec2& in_viewport_offset);
		void set_viewport_size(const glm::vec2& in_viewport_size);

		object_window* get_window() const { return m_window_render_on; }

	private:
		state_render_scene* m_render_scene_state = nullptr;
		object_window* m_window_render_on = nullptr;

		//offset in percentage(0 - 1) based on window size, top-left
		glm::vec2 m_viewport_offset = { 0.0f, 0.0f };
		//size in percentage(0 - 1) based on window size, top-left
		glm::vec2 m_viewport_size = { 1.0f, 1.0f };

		update_list_id<render_object_view> m_render_object_id;
		component_transform::on_updated::handle m_on_updated_handle;
	};

	struct object_view : public i_object<object_view, component_view, component_transform> {};

}
#pragma once
#include "impuls/component.h"

#include "impuls/component_transform.h"
#include "impuls/gl_includes.h"
#include "impuls/object.h"
#include "impuls/state_render_scene.h"

#include "glm/vec3.hpp"

namespace impuls
{
	struct State_render_scene;
	struct Object_window;

	struct Component_view : public Component
	{
		friend struct System_view;

		void set_window(Object_window* in_window);
		void set_viewport_offset(const glm::vec2& in_viewport_offset);
		void set_viewport_size(const glm::vec2& in_viewport_size);

		Object_window* get_window() const { return m_window_render_on; }

	private:
		State_render_scene* m_render_scene_state = nullptr;
		Object_window* m_window_render_on = nullptr;

		//offset in percentage(0 - 1) based on window size, top-left
		glm::vec2 m_viewport_offset = { 0.0f, 0.0f };
		//size in percentage(0 - 1) based on window size, top-left
		glm::vec2 m_viewport_size = { 1.0f, 1.0f };

		Update_list_id<Render_object_view> m_render_object_id;
		Component_transform::on_updated::Handle m_on_updated_handle;
	};

	struct object_view : public Object<object_view, Component_view, Component_transform> {};

}
#pragma once
#include "sic/component.h"

#include "sic/component_transform.h"
#include "sic/gl_includes.h"
#include "sic/object.h"
#include "sic/state_render_scene.h"
#include "sic/system_window.h"

#include "glm/vec3.hpp"

namespace sic
{
	struct State_render_scene;
	struct Object_window;

	struct Component_view : public Component
	{
		friend struct System_view;

		void set_window(Object_window* in_window);
		void set_viewport_offset(const glm::vec2& in_viewport_offset);
		void set_viewport_size(const glm::vec2& in_viewport_size);

		void set_fov(float in_fov);
		void set_near_and_far_plane(float in_near, float in_far);

		Object_window* get_window() const { return m_window_render_on; }

	private:
		State_render_scene* m_render_scene_state = nullptr;
		Object_window* m_window_render_on = nullptr;

		//offset in percentage(0 - 1) based on window size, top-left
		glm::vec2 m_viewport_offset = { 0.0f, 0.0f };
		//size in percentage(0 - 1) based on window size, top-left
		glm::vec2 m_viewport_size = { 1.0f, 1.0f };

		float m_fov = 45.0f;
		float m_near_plane = 0.1f;
		float m_far_plane = 100.0f;

		Update_list_id<Render_object_view> m_render_object_id;
		Component_transform::On_updated::Handle m_on_updated_handle;
		Component_window::On_Window_Created::Handle m_on_window_created_handle;
	};

	struct Object_view : public Object<Object_view, Component_view, Component_transform> {};

}
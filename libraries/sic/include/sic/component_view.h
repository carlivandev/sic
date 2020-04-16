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

	struct Component_view : public Component
	{
		friend struct System_view;

		void set_window(Window_proxy* in_window);

		//pixel size
		void set_viewport_dimensions(const glm::ivec2& in_viewport_dimensions);
		//offset in percentage(0 - 1) based on window size, bottom-left
		void set_viewport_offset(const glm::vec2& in_viewport_offset);
		//size in percentage(0 - 1) based on window size, bottom-left
		void set_viewport_size(const glm::vec2& in_viewport_size);

		void set_fov(float in_fov);
		void set_near_and_far_plane(float in_near, float in_far);

		void set_clear_color(const glm::vec4& in_clear_color);

		Window_proxy* get_window() const { return m_window; }

		glm::mat4 calculate_projection_matrix() const;

	private:
		State_render_scene* m_render_scene_state = nullptr;
		Window_proxy* m_window = nullptr;

		//pixel size
		glm::ivec2 m_viewport_dimensions = { 4, 4 };
		//offset in percentage(0 - 1) based on window size, bottom-left
		glm::vec2 m_viewport_offset = { 0.0f, 0.0f };
		//size in percentage(0 - 1) based on window size, bottom-left
		glm::vec2 m_viewport_size = { 1.0f, 1.0f };

		float m_fov = 45.0f;
		float m_near_plane = 0.1f;
		float m_far_plane = 100.0f;

		glm::vec4 m_clear_color = { 0.0f, 0.0f, 0.0f, 0.0f };

		Render_object_id<Render_object_view> m_render_object_id;
		Component_transform::On_updated::Handle m_on_updated_handle;
		Window_proxy::On_destroyed::Handle m_on_window_destroyed_handle;
	};

	struct Object_view : public Object<Object_view, Component_view, Component_transform> {};

}
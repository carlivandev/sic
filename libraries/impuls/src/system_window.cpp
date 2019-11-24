#include "impuls/system_window.h"

#include "impuls/asset_types.h"
#include "impuls/event.h"
#include "impuls/view.h"
#include "impuls/gl_includes.h"
#include "impuls/system_model.h"
#include "impuls/transform.h"
#include "impuls/logger.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

namespace impuls_private
{
	using namespace impuls;

	void glfw_error(int in_error_code, const char* in_message)
	{
		IMPULS_LOG_E(g_log_renderer, "glfw error[{0}]: {1}", in_error_code, in_message);
	}

	void window_resized(GLFWwindow* in_window, i32 in_width, i32 in_height)
	{
		component_window* wdw = static_cast<component_window*>(glfwGetWindowUserPointer(in_window));

		if (!wdw)
			return;

		wdw->m_dimensions_x = in_width;
		wdw->m_dimensions_y = in_height;
	}

	void window_focused(GLFWwindow* in_window, int in_focused)
	{
		component_window* wdw = static_cast<component_window*>(glfwGetWindowUserPointer(in_window));

		if (!wdw)
			return;

		wdw->m_is_focused = in_focused != 0;
	}

	void window_scrolled(GLFWwindow* in_window, double in_xoffset, double in_yoffset)
	{
		component_window* wdw = static_cast<component_window*>(glfwGetWindowUserPointer(in_window));

		if (!wdw)
			return;

		wdw->m_scroll_offset_x += in_xoffset;
		wdw->m_scroll_offset_y += in_yoffset;
	}
}

void impuls::system_window::on_created(world_context&& in_context) const
{
	if (!glfwInit())
	{
		IMPULS_LOG_E(g_log_renderer, "Failed to initialize GLFW");
		return;
	}

	glfwSetErrorCallback(&impuls_private::glfw_error);

	in_context.register_component_type<component_window>("component_window", 4);
	in_context.register_object<object_window>("window", 4, 1);

	in_context.register_component_type<component_view>("component_view", 4);
	in_context.register_object<object_view>("view", 4);

	in_context.register_state<state_main_window>("state_main_window");

	in_context.listen<impuls::event_created<object_window>>
	(
		[](world_context&, object_window& new_window)
		{
			constexpr i32 dimensions_x = 1600;
			constexpr i32 dimensions_y = 800;

			glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
			glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL 

			component_window& new_window_component = new_window.get<component_window>();
			new_window_component.m_dimensions_x = dimensions_x;
			new_window_component.m_dimensions_y = dimensions_y;

			new_window_component.m_window = glfwCreateWindow(new_window_component.m_dimensions_x, new_window_component.m_dimensions_y, "impuls_test_game", NULL, NULL);

			if (new_window_component.m_window == NULL)
			{
				IMPULS_LOG_E(g_log_renderer, "Failed to open GLFW window. GPU not 3.3 compatible.");
				glfwTerminate();
				return;
			}

			glfwSetWindowUserPointer(new_window_component.m_window, &new_window_component);
			glfwSetWindowSizeCallback(new_window_component.m_window, &impuls_private::window_resized);
			glfwSetWindowFocusCallback(new_window_component.m_window, &impuls_private::window_focused);
			glfwSetScrollCallback(new_window_component.m_window, &impuls_private::window_scrolled);

			glfwMakeContextCurrent(new_window_component.m_window); // Initialize GLEW
			glewExperimental = true; // Needed in core profile
			if (glewInit() != GLEW_OK) {
				IMPULS_LOG_E(g_log_renderer, "Failed to initialize GLEW.");
				return;
			}

			// Enable depth test
			glEnable(GL_DEPTH_TEST);
			// Accept fragment if it closer to the camera than the former one
			glDepthFunc(GL_LESS);

			glEnable(GL_CULL_FACE);

			// Ensure we can capture the escape key being pressed below
			glfwSetInputMode(new_window_component.m_window, GLFW_STICKY_KEYS, GL_TRUE);

			glfwMakeContextCurrent(nullptr);
		}
	);
}

void impuls::system_window::on_tick(world_context&& in_context, float in_time_delta) const
{
	in_time_delta;

	state_main_window* main_window_state = in_context.get_state<state_main_window>();

	if (!main_window_state)
		return;

	for (component_view& component_view_it : in_context.components<component_view>())
	{
		if (!component_view_it.m_window_render_on)
			continue;

		component_window& render_window = component_view_it.m_window_render_on->get<component_window>();

		impuls::i32 current_window_x, current_window_y;
		glfwGetWindowSize(render_window.m_window, &current_window_x, &current_window_y);
		
		if (render_window.m_dimensions_x != current_window_x ||
			render_window.m_dimensions_y != current_window_y)
		{
			//handle resize
			glfwSetWindowSize(render_window.m_window, render_window.m_dimensions_x, render_window.m_dimensions_y);
		}

		if (render_window.m_cursor_pos_to_set.has_value())
		{
			render_window.m_cursor_pos = render_window.m_cursor_pos_to_set.value();
			glfwSetCursorPos(render_window.m_window, render_window.m_cursor_pos.x, render_window.m_cursor_pos.y);

			render_window.m_cursor_pos_to_set.reset();
		}

		bool needs_cursor_reset = false;

		if (render_window.m_input_mode_to_set.has_value())
		{
			if (render_window.m_current_input_mode != e_window_input_mode::disabled &&
				render_window.m_input_mode_to_set.value() == e_window_input_mode::disabled)
				needs_cursor_reset = true;

			render_window.m_current_input_mode = render_window.m_input_mode_to_set.value();
			render_window.m_input_mode_to_set.reset();

			switch (render_window.m_current_input_mode)
			{
			case e_window_input_mode::normal:
				glfwSetInputMode(render_window.m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				break;
			case e_window_input_mode::disabled:
				glfwSetInputMode(render_window.m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				break;
			case e_window_input_mode::hidden:
				glfwSetInputMode(render_window.m_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
				break;
			default:
				break;
			}
		}

		glfwPollEvents();

		double cursor_x, cursor_y;
		glfwGetCursorPos(render_window.m_window, &cursor_x, &cursor_y);

		if (needs_cursor_reset)
		{
			render_window.m_cursor_movement = { 0.0f, 0.0f };
		}
		else
		{
			render_window.m_cursor_movement.x = static_cast<float>(cursor_x) - render_window.m_cursor_pos.x;
			render_window.m_cursor_movement.y = static_cast<float>(cursor_y) - render_window.m_cursor_pos.y;
		}

		render_window.m_cursor_pos.x = static_cast<float>(cursor_x);
		render_window.m_cursor_pos.y = static_cast<float>(cursor_y);

		if (glfwWindowShouldClose(render_window.m_window) != 0)
		{
			if (main_window_state->m_window == component_view_it.m_window_render_on)
				in_context.m_world->destroy();
			else
				glfwDestroyWindow(render_window.m_window);
		}
	}

	glfwMakeContextCurrent(nullptr);
}

void impuls::system_window::on_end_simulation(world_context&& in_context) const
{
	for (component_view& component_view_it : in_context.components<component_view>())
	{
		if (!component_view_it.m_window_render_on)
			continue;

		component_window& render_window = component_view_it.m_window_render_on->get<component_window>();
		glfwDestroyWindow(render_window.m_window);
	}

	glfwTerminate();
}

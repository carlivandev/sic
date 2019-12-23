#include "sic/system_window.h"

#include "sic/asset_types.h"
#include "sic/event.h"
#include "sic/system_view.h"
#include "sic/gl_includes.h"
#include "sic/system_model.h"
#include "sic/component_transform.h"
#include "sic/logger.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

namespace impuls_private
{
	using namespace sic;

	void glfw_error(int in_error_code, const char* in_message)
	{
		SIC_LOG_E(g_log_renderer, "glfw error[{0}]: {1}", in_error_code, in_message);
	}

	void window_resized(GLFWwindow* in_window, i32 in_width, i32 in_height)
	{
		Component_window* wdw = static_cast<Component_window*>(glfwGetWindowUserPointer(in_window));

		if (!wdw)
			return;

		wdw->m_dimensions_x = in_width;
		wdw->m_dimensions_y = in_height;
	}

	void window_focused(GLFWwindow* in_window, int in_focused)
	{
		Component_window* wdw = static_cast<Component_window*>(glfwGetWindowUserPointer(in_window));

		if (!wdw)
			return;

		wdw->m_is_focused = in_focused != 0;
	}

	void window_scrolled(GLFWwindow* in_window, double in_xoffset, double in_yoffset)
	{
		Component_window* wdw = static_cast<Component_window*>(glfwGetWindowUserPointer(in_window));

		if (!wdw)
			return;

		wdw->m_scroll_offset_x += in_xoffset;
		wdw->m_scroll_offset_y += in_yoffset;
	}
}

void sic::System_window::on_created(Engine_context&& in_context)
{
	if (!glfwInit())
	{
		SIC_LOG_E(g_log_renderer, "Failed to initialize GLFW");
		return;
	}

	glfwSetErrorCallback(&impuls_private::glfw_error);

	in_context.create_subsystem<System_view>(*this);

	in_context.register_component_type<Component_window>("component_window", 4);
	in_context.register_object<Object_window>("window", 4, 1);

	in_context.register_state<State_window>("state_window");

	in_context.listen<event_destroyed<Component_window>>
	(
		[window_state = in_context.get_state<State_window>()](Engine_context&, Component_window& window)
		{
			if (window.m_window)
				window_state->push_window_to_destroy(window.m_window);
		}
	);
}

void sic::System_window::on_shutdown(Engine_context&& in_context)
{
	glfwTerminate();
}

void sic::System_window::on_tick(Level_context&& in_context, float in_time_delta) const
{
	in_time_delta;

	State_window* window_state = in_context.m_engine.get_state<State_window>();

	if (!window_state)
		return;

	for (GLFWwindow* window : window_state->m_windows_to_destroy)
		glfwDestroyWindow(window);

	in_context.for_each<Component_window>
	(
		[](Component_window& window)
		{
			if (window.m_window)
				return;

			glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
			glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL

			window.m_window = glfwCreateWindow(window.m_dimensions_x, window.m_dimensions_y, "impuls_test_game", NULL, NULL);

			if (window.m_window == NULL)
			{
				SIC_LOG_E(g_log_renderer, "Failed to open GLFW window. GPU not 3.3 compatible.");
				glfwTerminate();
				return;
			}

			glfwSetWindowUserPointer(window.m_window, &window);
			glfwSetWindowSizeCallback(window.m_window, &impuls_private::window_resized);
			glfwSetWindowFocusCallback(window.m_window, &impuls_private::window_focused);
			glfwSetScrollCallback(window.m_window, &impuls_private::window_scrolled);

			glfwMakeContextCurrent(window.m_window); // Initialize GLEW
			glewExperimental = true; // Needed in core profile
			if (glewInit() != GLEW_OK) {
				SIC_LOG_E(g_log_renderer, "Failed to initialize GLEW.");
				return;
			}

			// Enable depth test
			glEnable(GL_DEPTH_TEST);
			// Accept fragment if it closer to the camera than the former one
			glDepthFunc(GL_LESS);

			glEnable(GL_CULL_FACE);

			// Ensure we can capture the escape key being pressed below
			glfwSetInputMode(window.m_window, GLFW_STICKY_KEYS, GL_TRUE);

			window.m_on_window_created.invoke(window.m_window);

			glfwMakeContextCurrent(nullptr);
		}
	);

	in_context.for_each<Component_window>
	(
		[&in_context, window_state](Component_window& window)
		{
			sic::i32 current_window_x, current_window_y;
			glfwGetWindowSize(window.m_window, &current_window_x, &current_window_y);

			if (window.m_dimensions_x != current_window_x ||
				window.m_dimensions_y != current_window_y)
			{
				//handle resize
				glfwSetWindowSize(window.m_window, window.m_dimensions_x, window.m_dimensions_y);
			}

			if (window.m_cursor_pos_to_set.has_value())
			{
				window.m_cursor_pos = window.m_cursor_pos_to_set.value();
				glfwSetCursorPos(window.m_window, window.m_cursor_pos.x, window.m_cursor_pos.y);

				window.m_cursor_pos_to_set.reset();
			}

			bool needs_cursor_reset = false;

			if (window.m_input_mode_to_set.has_value())
			{
				if (window.m_current_input_mode != Window_input_mode::disabled &&
					window.m_input_mode_to_set.value() == Window_input_mode::disabled)
					needs_cursor_reset = true;

				window.m_current_input_mode = window.m_input_mode_to_set.value();
				window.m_input_mode_to_set.reset();

				switch (window.m_current_input_mode)
				{
				case Window_input_mode::normal:
					glfwSetInputMode(window.m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
					break;
				case Window_input_mode::disabled:
					glfwSetInputMode(window.m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
					break;
				case Window_input_mode::hidden:
					glfwSetInputMode(window.m_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
					break;
				default:
					break;
				}
			}

			glfwPollEvents();

			double cursor_x, cursor_y;
			glfwGetCursorPos(window.m_window, &cursor_x, &cursor_y);

			if (needs_cursor_reset)
			{
				window.m_cursor_movement = { 0.0f, 0.0f };
			}
			else
			{
				window.m_cursor_movement.x = static_cast<float>(cursor_x) - window.m_cursor_pos.x;
				window.m_cursor_movement.y = static_cast<float>(cursor_y) - window.m_cursor_pos.y;
			}

			window.m_cursor_pos.x = static_cast<float>(cursor_x);
			window.m_cursor_pos.y = static_cast<float>(cursor_y);

			if (glfwWindowShouldClose(window.m_window) != 0)
			{
				if (&(window_state->m_main_window->get<Component_window>()) == &window)
					in_context.m_engine.shutdown();
				else
					in_context.destroy_object(window.owner());
			}
		}
	);

	glfwMakeContextCurrent(nullptr);
}
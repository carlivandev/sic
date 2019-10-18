#pragma once
#include "system.h"
#include "world_context.h"
#include "input.h"
#include "gl_includes.h"

struct GLFWwindow;

namespace impuls
{
	struct component_window : public i_component
	{
		i32 m_dimensions_x = 1600;
		i32 m_dimensions_y = 800;
		bool m_is_focused = true;
		double m_scroll_offset_x = 0.0;
		double m_scroll_offset_y = 0.0;

		GLFWwindow* m_window = nullptr;
	};

	struct object_window : public i_object<component_window> {};

	struct state_main_window : public i_state
	{
		object_window* m_window = nullptr;
	};

	struct system_window : i_system
	{
		//create window, create window state
		virtual void on_created(world_context&& in_context) const override;

		//poll window events
		virtual void on_tick(world_context&& in_context, float in_time_delta) const override;

		//cleanup
		virtual void on_end_simulation(world_context&& in_context) const override;
	};
}
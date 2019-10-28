#pragma once
#include "system.h"
#include "world_context.h"
#include "input.h"

#include "gl_includes.h"
#include "glm/vec2.hpp"

#include <optional>

struct GLFWwindow;

namespace impuls
{
	enum class e_window_input_mode
	{
		normal,
		disabled,
		hidden
	};

	struct component_window : public i_component
	{
		friend struct system_window;

		i32 m_dimensions_x = 1600;
		i32 m_dimensions_y = 800;
		bool m_is_focused = true;
		double m_scroll_offset_x = 0.0;
		double m_scroll_offset_y = 0.0;

		GLFWwindow* m_window = nullptr;

		void set_cursor_position(const glm::vec2& in_cursor_pos)
		{
			m_cursor_pos_to_set = in_cursor_pos;
		}

		const glm::vec2& get_cursor_position() const
		{
			return m_cursor_pos_to_set.value_or(m_cursor_pos);
		}

		void set_input_mode(e_window_input_mode in_mode)
		{
			m_input_mode_to_set = in_mode;
		}

		e_window_input_mode get_input_mode() const
		{
			return m_input_mode_to_set.value_or(m_current_input_mode);
		}

		const glm::vec2& get_cursor_movement() const { return m_cursor_movement; }

	private:
		std::optional<e_window_input_mode> m_input_mode_to_set;
		e_window_input_mode m_current_input_mode;

		std::optional<glm::vec2> m_cursor_pos_to_set;
		glm::vec2 m_cursor_pos;

		glm::vec2 m_cursor_movement;
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
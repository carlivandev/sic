#pragma once
#include "impuls/system.h"
#include "impuls/engine_context.h"
#include "impuls/object.h"
#include "impuls/input.h"

#include "impuls/gl_includes.h"
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

	struct Component_window : public Component
	{
		friend struct System_window;

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
		e_window_input_mode m_current_input_mode = e_window_input_mode::normal;

		std::optional<glm::vec2> m_cursor_pos_to_set;
		glm::vec2 m_cursor_pos;

		glm::vec2 m_cursor_movement;
	};

	struct Object_window : public Object<Object_window, Component_window> {};

	struct State_main_window : public State
	{
		Object_window* m_window = nullptr;
	};

	struct System_window : System
	{
		//create window, create window state
		virtual void on_created(Engine_context&& in_context) override;

		//poll window events
		virtual void on_tick(Level_context&& in_context, float in_time_delta) const override;

		//cleanup
		virtual void on_end_simulation(Level_context&& in_context) const override;
	};
}
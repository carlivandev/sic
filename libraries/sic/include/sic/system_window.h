#pragma once
#include "sic/system.h"
#include "sic/engine_context.h"
#include "sic/object.h"
#include "sic/input.h"
#include "sic/delegate.h"

#include "sic/gl_includes.h"
#include "glm/vec2.hpp"

#include <optional>

struct GLFWwindow;

namespace sic
{
	enum struct Window_input_mode
	{
		normal,
		disabled,
		hidden
	};


	struct Component_window : public Component
	{
		struct On_Window_Created : Delegate<GLFWwindow*> {};

		friend struct System_window;

		glm::ivec2 m_dimensions = { 1600, 800 };
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

		void set_input_mode(Window_input_mode in_mode)
		{
			m_input_mode_to_set = in_mode;
		}

		Window_input_mode get_input_mode() const
		{
			return m_input_mode_to_set.value_or(m_current_input_mode);
		}

		const glm::vec2& get_cursor_movement() const { return m_cursor_movement; }

		On_Window_Created m_on_window_created;

	private:
		std::optional<Window_input_mode> m_input_mode_to_set;
		Window_input_mode m_current_input_mode = Window_input_mode::normal;

		std::optional<glm::vec2> m_cursor_pos_to_set;
		glm::vec2 m_cursor_pos;

		glm::vec2 m_cursor_movement;
	};

	struct Object_window : public Object<Object_window, Component_window> {};

	struct State_window : public State
	{
		friend struct System_window;

		void push_window_to_destroy(GLFWwindow* in_window)
		{
			std::scoped_lock lock(m_mutex);
			m_windows_to_destroy.push_back(in_window);
		}

		Object_window* m_main_window = nullptr;

	private:
		std::vector<GLFWwindow*> m_windows_to_destroy;
		std::mutex m_mutex;
	};

	struct System_window : System
	{
		//create window, create window state
		virtual void on_created(Engine_context&& in_context) override;

		//cleanup
		virtual void on_shutdown(Engine_context&& in_context);

		//poll window events
		virtual void on_engine_tick(Engine_context&& in_context, float in_time_delta) const override;
	};
}
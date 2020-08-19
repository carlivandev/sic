#pragma once
#include "sic/system.h"
#include "sic/engine_context.h"
#include "sic/object.h"
#include "sic/input.h"
#include "sic/delegate.h"
#include "sic/update_list.h"
#include "sic/delegate.h"

#include "sic/gl_includes.h"
#include "glm/vec2.hpp"

#include <optional>
#include <string>

struct GLFWwindow;

namespace sic
{
	struct Render_object_window;

	enum struct Window_input_mode
	{
		normal,
		disabled,
		hidden
	};
	struct Window_proxy : Noncopyable
	{
		struct On_destroyed : Delegate<> {};

		friend struct System_window;
		friend struct State_window;
		friend struct System_window_functions;

		void set_dimensions(const glm::ivec2& in_dimensions);
		const glm::ivec2& get_dimensions() const { return m_dimensions; }

		void set_cursor_position(const glm::vec2& in_cursor_position);
		void set_cursor_position_internal(const glm::vec2& in_cursor_position) { m_cursor_position = in_cursor_position; }

		void set_input_mode(Window_input_mode in_input_mode);

		const glm::vec2& get_cursor_movement() const { return m_cursor_movement; }
		const std::string& get_name() const { return m_name; }

		bool m_is_focused = true;
		double m_scroll_offset_x = 0.0;
		double m_scroll_offset_y = 0.0;

		Update_list_id<Render_object_window> m_window_id;

		On_destroyed m_on_destroyed;

	private:
		Window_input_mode m_input_mode = Window_input_mode::normal;
		Engine_context m_engine_context;

		std::string m_name;
		glm::ivec2 m_dimensions = { 1600, 800 };
		glm::vec2 m_cursor_position = { 0.0f, 0.0f };
		glm::vec2 m_cursor_movement = { 0.0f, 0.0f };
		bool m_needs_cursor_reset = false;
	};

	struct State_window : public State
	{
		friend struct System_window;

		Window_proxy& create_window(Engine_context in_context, const std::string& in_name, const glm::ivec2& in_dimensions);
		void destroy_window(Engine_context in_context, const std::string& in_name);

		Window_proxy* find_window(const char* in_name) const;
		Window_proxy* get_focused_window() const;

		Window_proxy* m_main_window_interface = nullptr;
		GLFWwindow* m_resource_context = nullptr;
	private:
		std::unordered_map<std::string, std::unique_ptr<Window_proxy>> m_window_name_to_interfaces_lut;
		std::mutex m_mutex;
	};

	struct System_window : System
	{
		//create window, create window state
		virtual void on_created(Engine_context in_context) override;

		//cleanup
		virtual void on_shutdown(Engine_context in_context);

		//poll window events
		virtual void on_engine_tick(Engine_context in_context, float in_time_delta) const override;
	};
}
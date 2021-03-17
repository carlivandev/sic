#pragma once
#include "sic/input.h"
#include "sic/gl_includes.h"

#include "sic/core/system.h"
#include "sic/core/engine_context.h"
#include "sic/core/object.h"
#include "sic/core/delegate.h"
#include "sic/core/update_list.h"
#include "sic/core/delegate.h"

#include "glm/vec2.hpp"
#include "glm/vec4.hpp"

#include <optional>
#include <string>

struct GLFWwindow;

namespace sic
{
	struct Render_object_window;
	struct State_window;
	struct State_render_scene;
	struct State_ui;

	using Processor_window = Engine_processor<Processor_flag_write<State_window>, Processor_flag_write<State_render_scene>>;

	enum struct Window_input_mode
	{
		normal,
		disabled,
		hidden
	};

	struct Window_proxy : Noncopyable
	{
		struct On_destroyed : Delegate<Processor_window> {};

		friend struct System_window;
		friend struct State_window;
		friend struct System_window_functions;
		friend struct System_input;

		void set_dimensions(Processor_window in_processor, const glm::ivec2& in_dimensions);
		const glm::ivec2& get_dimensions() const { return m_dimensions; }

		void set_cursor_position(Engine_processor<> in_processor, const glm::vec2& in_cursor_position);
		void set_cursor_position_internal(const glm::vec2& in_cursor_position) { m_cursor_position = in_cursor_position; }

		void set_input_mode(Engine_processor<> in_processor, Window_input_mode in_input_mode);

		const glm::vec2& get_cursor_movement() const { return m_cursor_movement; }
		const glm::vec2& get_cursor_postition() const { return m_cursor_position; }
		const glm::vec2& get_monitor_position() const { return m_monitor_position; }

		const std::string& get_name() const { return m_name; }

		bool m_is_focused = true;
		bool m_is_being_dragged = false;
		double m_scroll_offset_x = 0.0;
		double m_scroll_offset_y = 0.0;
		float m_time_since_focused = 0.0f;

		Update_list_id<Render_object_window> m_window_id;

		On_destroyed m_on_destroyed;
		Engine_context m_engine_context;
	private:
		Window_input_mode m_input_mode = Window_input_mode::normal;

		std::string m_name;
		glm::ivec2 m_dimensions = { 1600, 800 };
		glm::vec2 m_cursor_position = { 0.0f, 0.0f };
		glm::vec2 m_cursor_movement = { 0.0f, 0.0f };
		glm::vec2 m_monitor_position = { 0.0f, 0.0f };
		glm::vec2 m_cursor_drag = { 0.0f, 0.0f };
		std::optional<glm::vec2> m_cursor_initial_drag_offset;
		std::optional<glm::vec2> m_resize_edge;
		std::optional<glm::vec4> m_resize_border;

		std::unordered_map<std::string, GLFWcursor*> m_cursors;
		bool m_needs_cursor_reset = false;
		bool m_is_maximized = false;
		bool m_being_moved = false;

		std::array<bool, static_cast<i32>(Key::count)> m_key_this_frame_down;
		std::optional<unsigned int> m_character_input;
	};

	struct State_window : public State
	{
		friend struct System_window;

		Window_proxy& create_window(Processor_window in_processor, const std::string& in_name, const glm::ivec2& in_dimensions, bool in_decorated);
		void destroy_window(Processor_window in_processor, const std::string& in_name);

		void minimize_window(Processor_window in_processor, const std::string& in_name);
		void toggle_maximize_window(Processor_window in_processor, const std::string& in_name);
		void set_window_position(Processor_window in_processor, const std::string& in_name, const glm::vec2& in_position);

		void begin_drag_window(Processor_window in_processor, const std::string& in_name);
		void end_drag_window(Processor_window in_processor, const std::string& in_name);

		void begin_resize(Processor_window in_processor, const std::string& in_name, const glm::vec2& in_resize_edge);
		void end_resize(Processor_window in_processor, const std::string& in_name);

		void set_cursor(Processor_window in_processor, const std::string& in_name, const std::string& in_cursor_key);
		void set_cursor_icon(Processor_window in_processor, const std::string& in_name, const std::string& in_cursor_key, const glm::ivec2& in_dimensions, const glm::ivec2& in_pointer_location, unsigned char* in_data);

		Window_proxy* find_window(const char* in_name) const;
		Window_proxy* get_focused_window() const;

		const std::unordered_map<std::string, std::unique_ptr<Window_proxy>>& get_windows() const { return m_window_name_to_interfaces_lut; }

		Window_proxy* m_main_window_interface = nullptr;
		GLFWwindow* m_resource_context = nullptr;
	private:
		std::unordered_map<std::string, std::unique_ptr<Window_proxy>> m_window_name_to_interfaces_lut;
		std::mutex m_mutex;
		i32 m_id_ticker = 0;
	};

	struct System_window : System
	{
		//create window, create window state
		virtual void on_created(Engine_context in_context) override;

		//cleanup
		virtual void on_shutdown(Engine_context in_context);

		//poll window events
		virtual void on_engine_tick(Engine_context in_context, float in_time_delta) const override;

		static void update_windows(Processor_window in_processor);

		static void update_dragging(Window_proxy& inout_window, const Render_object_window& in_window_ro);
	};
}
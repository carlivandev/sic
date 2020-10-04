#include "sic/system_window.h"

#include "sic/asset_types.h"
#include "sic/system_view.h"
#include "sic/gl_includes.h"
#include "sic/system_model.h"
#include "sic/component_transform.h"
#include "sic/shader_parser.h"
#include "sic/system_ui.h"
#include "sic/state_render_scene.h"
#include "sic/system_renderer.h"

#include "sic/core/event.h"
#include "sic/core/logger.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

namespace sic
{
	struct System_window_functions
	{
		static void glfw_error(int in_error_code, const char* in_message)
		{
			SIC_LOG_E(g_log_renderer, "glfw error[{0}]: {1}", in_error_code, in_message);
		}

		static void gl_debug_message_callback
		(
			GLenum in_source,
			GLenum in_type,
			GLuint in_id,
			GLenum in_severity,
			GLsizei in_length,
			const GLchar* in_message,
			const void* in_user_param
		)
		{
			in_source;
			in_length;
			in_user_param;

			const char* format_string =
				"OpenGl_message:\n"
				"Message: {0}\n"
				"Type: {1}\n"
				"ID: {2}\n"
				"Severity: {3}\n"
				;

			std::string type_string;

			switch (in_type)
			{
			case GL_DEBUG_TYPE_ERROR:
				type_string = "ERROR";
				break;
			case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
				type_string = "DEPRECATED_BEHAVIOR";
				break;
			case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
				type_string = "UNDEFINED_BEHAVIOR";
				break;
			case GL_DEBUG_TYPE_PORTABILITY:
				type_string = "PORTABILITY";
				break;
			case GL_DEBUG_TYPE_PERFORMANCE:
				type_string = "PERFORMANCE";
				break;
			case GL_DEBUG_TYPE_OTHER:
				type_string = "OTHER";
				break;
			}

			std::string severity_string;

			switch (in_severity)
			{
			case GL_DEBUG_SEVERITY_LOW:
				severity_string = "LOW";
				break;
			case GL_DEBUG_SEVERITY_MEDIUM:
				severity_string = "MEDIUM";
				break;
			case GL_DEBUG_SEVERITY_HIGH:
				severity_string = "HIGH";
				break;
			default:
				severity_string = "OTHER";
				break;
			}

			switch (in_type)
			{
			case GL_DEBUG_TYPE_ERROR:
			case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
				SIC_LOG_E(g_log_renderer, format_string, in_message, type_string, in_id, severity_string);
				break;
			case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
			case GL_DEBUG_TYPE_PORTABILITY:
			case GL_DEBUG_TYPE_PERFORMANCE:
				SIC_LOG_W(g_log_renderer, format_string, in_message, type_string, in_id, severity_string);
				break;
			case GL_DEBUG_TYPE_OTHER:
				SIC_LOG(g_log_renderer_verbose, format_string, in_message, type_string, in_id, severity_string);
				break;
			default:
				break;
			}
		}

		static void window_focused(GLFWwindow* in_window, int in_focused)
		{
			Window_proxy* wdw = static_cast<Window_proxy*>(glfwGetWindowUserPointer(in_window));

			if (!wdw)
				return;

			wdw->m_is_focused = in_focused != 0;

			// focused window changed, need to clear input
		}

		static void window_scrolled(GLFWwindow* in_window, double in_xoffset, double in_yoffset)
		{
			Window_proxy* wdw = static_cast<Window_proxy*>(glfwGetWindowUserPointer(in_window));

			if (!wdw)
				return;

			wdw->m_scroll_offset_x += in_xoffset;
			wdw->m_scroll_offset_y += in_yoffset;
		}

		static void window_resized(GLFWwindow* in_window, int in_width, int in_height)
		{
			Window_proxy* wdw = static_cast<Window_proxy*>(glfwGetWindowUserPointer(in_window));

			if (!wdw)
				return;

			if (wdw->m_dimensions.x < in_width && wdw->m_dimensions.y < in_height)
				wdw->m_is_maximized = false;
		}

		static void window_moved(GLFWwindow* in_window, int in_x, int in_y)
		{
			Window_proxy* wdw = static_cast<Window_proxy*>(glfwGetWindowUserPointer(in_window));

			if (!wdw)
				return;

			if (wdw->m_being_moved)
				return;

			wdw->m_monitor_position.x = (float)in_x;
			wdw->m_monitor_position.y = (float)in_y;
		}

		static void cursor_moved(GLFWwindow* in_window, double in_x, double in_y)
		{
			Window_proxy* wdw = static_cast<Window_proxy*>(glfwGetWindowUserPointer(in_window));

			if (!wdw)
				return;

			if (wdw->m_cursor_initial_drag_offset.has_value())
			{
				wdw->m_cursor_drag.x = in_x - wdw->m_cursor_initial_drag_offset.value().x;
				wdw->m_cursor_drag.y = in_y - wdw->m_cursor_initial_drag_offset.value().y;

				SIC_LOG_W(g_log_game, "x: {0}, y: {1}", wdw->m_cursor_drag.x, wdw->m_cursor_drag.y);
			}
		}

		static void mousebutton(GLFWwindow* in_window, int in_button, int in_action, int in_mods)
		{
			Window_proxy* wdw = static_cast<Window_proxy*>(glfwGetWindowUserPointer(in_window));

			if (!wdw)
				return;

			if (in_action == GLFW_PRESS && !wdw->m_cursor_initial_drag_offset.has_value())
			{
				double x, y;
				glfwGetCursorPos(in_window, &x, &y);

				wdw->m_cursor_initial_drag_offset = { (float)glm::floor(x), (float)glm::floor(y) };
			}
			else if (in_action == GLFW_RELEASE)
			{
				wdw->m_cursor_initial_drag_offset.reset();
			}
		}
	};
}

void sic::System_window::on_created(Engine_context in_context)
{
	if (!glfwInit())
	{
		SIC_LOG_E(g_log_renderer, "Failed to initialize GLFW");
		return;
	}

	in_context.create_subsystem<System_view>(*this);

	in_context.register_state<State_window>("state_window");

	State_window& window_state = in_context.get_state_checked<State_window>();
	
	glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

	window_state.m_resource_context = glfwCreateWindow(1, 1, "resource_context", NULL, nullptr);

	if (window_state.m_resource_context == NULL)
	{
		SIC_LOG_E(g_log_renderer, "Failed to open GLFW window. GPU not 3.3 compatible.");
		glfwTerminate();
		return;
	}
	glfwMakeContextCurrent(window_state.m_resource_context);
	glewExperimental = true; // Needed in core profile

	if (glewInit() != GLEW_OK)
	{
		SIC_LOG_E(g_log_renderer, "Failed to initialize GLEW.");
		return;
	}

	glfwSetErrorCallback(&System_window_functions::glfw_error);

	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(&System_window_functions::gl_debug_message_callback, nullptr);
	GLuint unusedIds = 0;
	glDebugMessageControl(GL_DONT_CARE,
		GL_DONT_CARE,
		GL_DONT_CARE,
		0,
		&unusedIds,
		true);

}

void sic::System_window::on_shutdown(Engine_context)
{
	glfwTerminate();
}

void sic::System_window::on_engine_tick(Engine_context in_context, float in_time_delta) const
{
	in_time_delta;
	in_context.schedule(update_windows, Schedule_data().run_on_main_thread(true));
}

void sic::System_window::update_windows(Processor_window in_processor)
{
	State_window& window_state = in_processor.get_state_checked_w<State_window>();

	State_render_scene& scene_state = in_processor.get_state_checked_w<State_render_scene>();

	Window_proxy* focused_window = window_state.get_focused_window();

	glfwMakeContextCurrent(window_state.m_resource_context);
	glfwPollEvents();

	if (focused_window)
	{
		glm::dvec2 new_pos;

		const Render_object_window* focused_window_ro = scene_state.m_windows.find_object(focused_window->m_window_id);
		if (focused_window_ro)
		{
			glfwGetCursorPos(focused_window_ro->m_context, &new_pos.x, &new_pos.y);

			const glm::vec2 prev_pos = focused_window->m_cursor_position;

			focused_window->m_cursor_position = new_pos;

			if (focused_window->m_needs_cursor_reset)
			{
				focused_window->m_cursor_movement = { 0.0f, 0.0f };
				focused_window->m_needs_cursor_reset = false;
			}
			else
			{
				focused_window->m_cursor_movement = focused_window->m_cursor_position - prev_pos;
			}

			update_dragging(*focused_window, *focused_window_ro);

			glm::ivec2 window_dimensions;
			glfwGetWindowSize(focused_window_ro->m_context, &window_dimensions.x, &window_dimensions.y);

			if (focused_window->m_dimensions != window_dimensions)
			{
				if (window_dimensions.x != 0 && window_dimensions.y != 0)
				{
					focused_window->m_dimensions = window_dimensions;

					in_processor.update_state_deferred<State_ui>
					(
						[name = focused_window->get_name(), id = focused_window->m_window_id, window_dimensions](State_ui& inout_ui_state)
						{
							Ui_widget_canvas* canvas = inout_ui_state.find<Ui_widget_canvas>(name);
							assert(canvas);

							canvas->m_reference_dimensions = window_dimensions;
							inout_ui_state.set_window_info(name, window_dimensions, id);
						}
					);
				}
			}
		}
	}

	glfwMakeContextCurrent(window_state.m_resource_context);

	for (Render_object_window& window : scene_state.m_windows.m_objects)
	{
		if (glfwWindowShouldClose(window.m_context) != 0)
		{
			const Render_object_window* main_window = scene_state.m_windows.find_object(window_state.m_main_window_interface->m_window_id);
			
			auto to_destroy_it = window_state.m_window_name_to_interfaces_lut.find(window.m_name);

			if (to_destroy_it != window_state.m_window_name_to_interfaces_lut.end())
				window_state.destroy_window(in_processor, to_destroy_it->first);
		}
		else
		{
			glm::ivec2 window_dimensions;
			glfwGetWindowSize(window.m_context, &window_dimensions.x, &window_dimensions.y);

			if (window.m_render_target.value().get_dimensions() != window_dimensions)
			{
				if (window_dimensions.x != 0 && window_dimensions.y != 0)
				{
					window.m_render_target.value().resize(window_dimensions);
					window.m_ui_render_target.value().resize(window_dimensions * 2);
				}
			}
		}
	}

	glfwMakeContextCurrent(nullptr);
}

void sic::System_window::update_dragging(Window_proxy& inout_window, const Render_object_window& in_window_ro)
{
	if (!inout_window.m_cursor_initial_drag_offset.has_value())
		return;

	if (inout_window.m_cursor_drag.x == 0.0f && inout_window.m_cursor_drag.y == 0.0f)
		return;

	if (inout_window.m_is_being_dragged)
	{
		if (inout_window.m_is_maximized)
		{
			inout_window.m_is_maximized = false;

			const glm::vec2 offset_percentage = inout_window.m_cursor_initial_drag_offset.value() / glm::vec2(inout_window.m_dimensions.x, inout_window.m_dimensions.y);
			const glm::ivec2 old_cursor_pos_on_monitor = inout_window.m_monitor_position + inout_window.m_cursor_initial_drag_offset.value();

			glfwRestoreWindow(in_window_ro.m_context);

			glm::ivec2 window_dimensions;
			glfwGetWindowSize(in_window_ro.m_context, &window_dimensions.x, &window_dimensions.y);

			inout_window.m_cursor_initial_drag_offset.value().x = window_dimensions.x * offset_percentage.x;
			inout_window.m_cursor_initial_drag_offset.value().y = window_dimensions.y * offset_percentage.y;

			glm::ivec2 new_pos = old_cursor_pos_on_monitor;
			new_pos += glm::ivec2((offset_percentage.x * window_dimensions.x), (offset_percentage.y * window_dimensions.y));
			glfwSetWindowPos(in_window_ro.m_context, new_pos.x, new_pos.y);
		}

		int w_x, w_y;
		glfwGetWindowPos(in_window_ro.m_context, &w_x, &w_y);

		const glm::ivec2 new_pos =
		{
			w_x + inout_window.m_cursor_drag.x,
			w_y + inout_window.m_cursor_drag.y
		};

		glfwSetWindowPos(in_window_ro.m_context, new_pos.x, new_pos.y);
	}
	else if (inout_window.m_resize_edge.has_value())
	{
		glfwMakeContextCurrent(in_window_ro.m_context);

		glm::ivec2 old_pos;
		glfwGetWindowPos(in_window_ro.m_context, &old_pos.x, &old_pos.y);

		glm::ivec2 old_size;
		glfwGetWindowSize(in_window_ro.m_context, &old_size.x, &old_size.y);

		const glm::ivec2 new_size =
		{
			glm::max(old_size.x + (inout_window.m_resize_edge.value().x * inout_window.m_cursor_drag.x), 200.0f),
			glm::max(old_size.y + (inout_window.m_resize_edge.value().y * inout_window.m_cursor_drag.y), 200.0f)
		};

		const glm::ivec2 new_pos =
		{
			old_pos.x + (inout_window.m_resize_edge.value().x < 0.0f ? old_size.x - new_size.x : 0.0f),
			old_pos.y + (inout_window.m_resize_edge.value().y < 0.0f ? old_size.y - new_size.y : 0.0f)
		};

		glfwSetWindowSize(in_window_ro.m_context, new_size.x, new_size.y);
		glfwSetWindowPos(in_window_ro.m_context, new_pos.x, new_pos.y);

		inout_window.m_cursor_initial_drag_offset.value().x -= (inout_window.m_resize_edge.value().x > 0.0f ? old_size.x - new_size.x : 0.0f);
		inout_window.m_cursor_initial_drag_offset.value().y -= (inout_window.m_resize_edge.value().y > 0.0f ? old_size.y - new_size.y : 0.0f);
	}

	inout_window.m_cursor_drag = { 0.0f, 0.0f };
}

sic::Window_proxy& sic::State_window::create_window(Processor_window in_processor, const std::string& in_name, const glm::ivec2& in_dimensions)
{
	std::scoped_lock lock(m_mutex);

	const glm::ivec2 clamped_dimensions = glm::max(in_dimensions, glm::ivec2(200, 200));

	State_render_scene& scene_state = in_processor.get_state_checked_w<State_render_scene>();

	auto& window_interface_ptr = m_window_name_to_interfaces_lut[in_name];

	if (window_interface_ptr.get())
		return *window_interface_ptr;

	window_interface_ptr = std::make_unique<Window_proxy>();
	auto window_interface_ptr_raw = window_interface_ptr.get();

	if (m_main_window_interface == nullptr)
		m_main_window_interface = window_interface_ptr_raw;

	window_interface_ptr_raw->m_name = in_name;
	window_interface_ptr_raw->m_dimensions = clamped_dimensions;
	window_interface_ptr_raw->m_engine_context = Engine_context(*in_processor.m_engine);

	window_interface_ptr_raw->m_window_id = scene_state.m_windows.create_object
	(
		[window_interface_ptr_raw, in_name, clamped_dimensions, resource_context = m_resource_context](Render_object_window& in_out_window)
		{
			glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
			glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL
			glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
			glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

			in_out_window.m_context = glfwCreateWindow(clamped_dimensions.x, clamped_dimensions.y, in_name.c_str(), NULL, resource_context);
			in_out_window.m_name = in_name;

			if (in_out_window.m_context == NULL)
			{
				SIC_LOG_E(g_log_renderer, "Failed to open GLFW window. GPU not 3.3 compatible.");
				glfwTerminate();
				return;
			}

			glfwSetWindowUserPointer(in_out_window.m_context, window_interface_ptr_raw);
			glfwSetWindowSizeCallback(in_out_window.m_context, &System_window_functions::window_resized);
			glfwSetWindowFocusCallback(in_out_window.m_context, &System_window_functions::window_focused);
			glfwSetWindowPosCallback(in_out_window.m_context, &System_window_functions::window_moved);
			glfwSetScrollCallback(in_out_window.m_context, &System_window_functions::window_scrolled);
			glfwSetCursorPosCallback(in_out_window.m_context, &System_window_functions::cursor_moved);
			glfwSetMouseButtonCallback(in_out_window.m_context, &System_window_functions::mousebutton);

			glfwMakeContextCurrent(in_out_window.m_context); // Initialize GLEW
			glewExperimental = true; // Needed in core profile
			if (glewInit() != GLEW_OK)
			{
				SIC_LOG_E(g_log_renderer, "Failed to initialize GLEW.");
				return;
			}

			glfwSetErrorCallback(&System_window_functions::glfw_error);

			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
			glDebugMessageCallback(&System_window_functions::gl_debug_message_callback, nullptr);
			GLuint unusedIds = 0;
			glDebugMessageControl(GL_DONT_CARE,
				GL_DONT_CARE,
				GL_DONT_CARE,
				0,
				&unusedIds,
				true);

			glEnable(GL_DEBUG_OUTPUT);

			//glfwSwapInterval(0);

			// Ensure we can capture the escape key being pressed below
			glfwSetInputMode(in_out_window.m_context, GLFW_STICKY_KEYS, GL_TRUE);

			window_interface_ptr_raw->m_cursors["arrow"] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
			window_interface_ptr_raw->m_cursors["resize_ew"] = glfwCreateStandardCursor(GLFW_RESIZE_EW_CURSOR);
			window_interface_ptr_raw->m_cursors["resize_ns"] = glfwCreateStandardCursor(GLFW_RESIZE_NS_CURSOR);
			window_interface_ptr_raw->m_cursors["resize_nesw"] = glfwCreateStandardCursor(GLFW_RESIZE_NESW_CURSOR);
			window_interface_ptr_raw->m_cursors["resize_nwse"] = glfwCreateStandardCursor(GLFW_RESIZE_NWSE_CURSOR);

			in_out_window.m_quad_vertex_buffer_array.emplace();

			const char* quad_vertex_shader_path = "content/engine/materials/pass_through.vert";
			const char* quad_fragment_shader_path = "content/engine/materials/simple_texture.frag";

			in_out_window.m_quad_program.emplace
			(
				quad_vertex_shader_path,
				Shader_parser::parse_shader(quad_vertex_shader_path).value(),
				quad_fragment_shader_path,
				Shader_parser::parse_shader(quad_fragment_shader_path).value()
			);
			in_out_window.m_quad_indexbuffer.emplace(OpenGl_buffer::Creation_params(OpenGl_buffer_target::element_array, OpenGl_buffer_usage::static_draw));
			const std::vector<GLfloat> positions = {
				-1.0f, -1.0f,
				-1.0f, 1.0f,
				1.0f, 1.0f,
				1.0f, -1.0f
			};

			const std::vector<GLfloat> tex_coords =
			{
				0.0f, 0.0f,
				0.0f, 1.0f,
				1.0f, 1.0f,
				1.0f, 0.0f
			};

			/*
			1 2
			0 3
			*/
			std::vector<unsigned int> indices = {
				0, 2, 1,
				0, 3, 2
			};

			in_out_window.m_quad_vertex_buffer_array.value().bind();

			in_out_window.m_quad_vertex_buffer_array.value().set_data<OpenGl_vertex_attribute_position2D>(positions);
			in_out_window.m_quad_vertex_buffer_array.value().set_data<OpenGl_vertex_attribute_texcoord>(tex_coords);

			in_out_window.m_quad_indexbuffer.value().set_data(indices);

			//we have to initialize fbo on main context cause it is not shared
			glfwMakeContextCurrent(resource_context);
			in_out_window.m_render_target.emplace(clamped_dimensions, OpenGl_texture_format::rgb,  false);
			in_out_window.m_ui_render_target.emplace(clamped_dimensions * 2, OpenGl_texture_format::rgba, false);
		}
	);

	in_processor.update_state_deferred<State_ui>
	(
		[in_name, clamped_dimensions, id = window_interface_ptr_raw->m_window_id](State_ui& inout_ui_state)
		{
			Ui_widget_canvas& canvas = inout_ui_state.create<Ui_widget_canvas>(in_name);
			canvas.m_reference_dimensions = clamped_dimensions;

			inout_ui_state.set_window_info(in_name, clamped_dimensions, id);
		}
	);

	return *window_interface_ptr;
}

void sic::State_window::destroy_window(Processor_window in_processor, const std::string& in_name)
{
	std::scoped_lock lock(m_mutex);

	auto window_interface_ptr = m_window_name_to_interfaces_lut.find(in_name);

	if (window_interface_ptr == m_window_name_to_interfaces_lut.end())
		return;

	window_interface_ptr->second->m_on_destroyed.invoke();

	State_render_scene& scene_state = in_processor.get_state_checked_w<State_render_scene>();
	Render_object_window* window_ro = scene_state.m_windows.find_object(window_interface_ptr->second->m_window_id);

	glfwMakeContextCurrent(window_ro->m_context);

	for (auto&& cursor_it : window_interface_ptr->second->m_cursors)
		glfwDestroyCursor(cursor_it.second);

	scene_state.m_windows.destroy_object(window_interface_ptr->second->m_window_id);

	in_processor.update_state_deferred<State_ui>
	(
		[name = window_interface_ptr->second->get_name()](State_ui& inout_ui_state)
		{
			inout_ui_state.destroy(name);
		}
	);

	if (window_interface_ptr->second.get() == m_main_window_interface)
		this_thread().update_deferred([](Engine_context in_context) { in_context.shutdown(); });

	m_window_name_to_interfaces_lut.erase(window_interface_ptr);
}

void sic::State_window::minimize_window(Processor_window in_processor, const std::string& in_name)
{
	Window_proxy* window = find_window(in_name.c_str());
	if (!window)
		return;

	State_render_scene& scene_state = in_processor.get_state_checked_w<State_render_scene>();

	const Render_object_window* window_ro = scene_state.m_windows.find_object(window->m_window_id);
	if (window_ro)
		glfwIconifyWindow(window_ro->m_context);
}

void sic::State_window::toggle_maximize_window(Processor_window in_processor, const std::string& in_name)
{
	Window_proxy* window = find_window(in_name.c_str());
	if (!window)
		return;

	State_render_scene& scene_state = in_processor.get_state_checked_w<State_render_scene>();

	const Render_object_window* window_ro = scene_state.m_windows.find_object(window->m_window_id);
	if (window_ro)
	{
		if (window->m_is_maximized)
			glfwRestoreWindow(window_ro->m_context);
		else
			glfwMaximizeWindow(window_ro->m_context);

		window->m_is_maximized = !window->m_is_maximized;
	}
}

void sic::State_window::set_window_position(Processor_window in_processor, const std::string& in_name, const glm::vec2& in_position)
{
	Window_proxy* window = find_window(in_name.c_str());
	if (!window)
		return;

	State_render_scene& scene_state = in_processor.get_state_checked_w<State_render_scene>();

	const Render_object_window* window_ro = scene_state.m_windows.find_object(window->m_window_id);
	if (window_ro)
	{
		if (window->m_is_maximized)
			glfwRestoreWindow(window_ro->m_context);

		window->m_is_maximized = false;
		window->m_being_moved = true;
		glfwSetWindowPos(window_ro->m_context, (int)in_position.x, (int)in_position.y);
		window->m_monitor_position = in_position;
		window->m_being_moved = false;
	}
}

void sic::State_window::begin_drag_window(Processor_window in_processor, const std::string& in_name)
{
	Window_proxy* window = find_window(in_name.c_str());
	if (!window)
		return;

	window->m_is_being_dragged = true;
}

void sic::State_window::end_drag_window(Processor_window in_processor, const std::string& in_name)
{
	Window_proxy* window = find_window(in_name.c_str());
	if (!window)
		return;

	window->m_is_being_dragged = false;
}

void sic::State_window::begin_resize(Processor_window in_processor, const std::string& in_name, const glm::vec2& in_resize_edge)
{
	Window_proxy* window = find_window(in_name.c_str());
	if (!window)
		return;

	window->m_resize_edge = in_resize_edge;
}

void sic::State_window::end_resize(Processor_window in_processor, const std::string& in_name)
{
	Window_proxy* window = find_window(in_name.c_str());
	if (!window)
		return;

	window->m_resize_edge.reset();
}

void sic::State_window::set_cursor(Processor_window in_processor, const std::string& in_name, const std::string& in_cursor_key)
{
	Window_proxy* window = find_window(in_name.c_str());
	if (!window)
		return;

	auto cursor_it = window->m_cursors.find(in_cursor_key);
	if (cursor_it == window->m_cursors.end())
		return;

	State_render_scene& scene_state = in_processor.get_state_checked_w<State_render_scene>();

	const Render_object_window* window_ro = scene_state.m_windows.find_object(window->m_window_id);
	if (window_ro)
		glfwSetCursor(window_ro->m_context, cursor_it->second);
}

void sic::State_window::set_cursor_icon(Processor_window in_processor, const std::string& in_name, const std::string& in_cursor_key, const glm::ivec2& in_dimensions, const glm::ivec2& in_pointer_location, unsigned char* in_data)
{
	Window_proxy* window = find_window(in_name.c_str());
	if (!window)
		return;

	State_render_scene& scene_state = in_processor.get_state_checked_w<State_render_scene>();

	const Render_object_window* window_ro = scene_state.m_windows.find_object(window->m_window_id);
	if (!window_ro)
		return;

	glfwMakeContextCurrent(window_ro->m_context);

	auto cur_cursor_it = window->m_cursors.find(in_cursor_key);

	if (cur_cursor_it != window->m_cursors.end())
		glfwDestroyCursor(cur_cursor_it->second);

	GLFWimage cursor;
	cursor.width = in_dimensions.x;
	cursor.height = in_dimensions.y;

	cursor.pixels = in_data;

	window->m_cursors[in_cursor_key] = glfwCreateCursor(&cursor, in_pointer_location.x, in_pointer_location.y);
}

sic::Window_proxy* sic::State_window::find_window(const char* in_name) const
{
	auto it = m_window_name_to_interfaces_lut.find(in_name);

	if (it == m_window_name_to_interfaces_lut.end())
		return nullptr;

	return it->second.get();
}

sic::Window_proxy* sic::State_window::get_focused_window() const
{
	for (auto&& it : m_window_name_to_interfaces_lut)
	{
		Window_proxy* wd = it.second.get();
		
		if (wd && wd->m_is_focused)
			return wd;
	}
	return nullptr;
}

void sic::Window_proxy::set_dimensions(Processor_window in_processor, const glm::ivec2& in_dimensions)
{
	m_dimensions = in_dimensions;

	State_render_scene& scene_state = in_processor.get_state_checked_w<State_render_scene>();
	auto resource_context = in_processor.get_state_checked_w<State_window>().m_resource_context;

	if (in_dimensions.x == 0 || in_dimensions.y == 0)
		return;

	scene_state.m_windows.update_object
	(
		m_window_id,
		[resource_context, in_dimensions](Render_object_window& inout_window)
		{
			glfwMakeContextCurrent(resource_context);

			if (inout_window.m_render_target.value().get_dimensions() != in_dimensions)
			{
				glfwSetWindowSize(inout_window.m_context, in_dimensions.x, in_dimensions.y);
				inout_window.m_render_target.value().resize(in_dimensions);
				inout_window.m_ui_render_target.value().resize(in_dimensions * 2);
			}
		}
	);
}

void sic::Window_proxy::set_cursor_position(Processor<Processor_flag_deferred_write<State_render_scene>> in_processor, const glm::vec2& in_cursor_position)
{
	m_cursor_position = in_cursor_position;

	in_processor.update_state_deferred<State_render_scene>
	(
		[window_id = m_window_id, in_cursor_position](State_render_scene& inout_state)
		{
			inout_state.m_windows.update_object
			(
				window_id,
				[in_cursor_position](Render_object_window& inout_window)
				{
					glfwSetCursorPos(inout_window.m_context, in_cursor_position.x, in_cursor_position.y);
				}
			);
		}
	);
}

void sic::Window_proxy::set_input_mode(Processor<Processor_flag_deferred_write<State_render_scene>> in_processor, Window_input_mode in_input_mode)
{
	m_input_mode = in_input_mode;

	in_processor.update_state_deferred<State_render_scene>
	(
		[window_id = m_window_id, &needs_cursor_reset = m_needs_cursor_reset, in_input_mode](State_render_scene& inout_state)
		{
			inout_state.m_windows.update_object
			(
				window_id,
				[&needs_cursor_reset, in_input_mode](Render_object_window& inout_window)
				{
					switch (in_input_mode)
					{
					case Window_input_mode::normal:
						glfwSetInputMode(inout_window.m_context, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
						break;
					case Window_input_mode::disabled:
						glfwSetInputMode(inout_window.m_context, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
						break;
					case Window_input_mode::hidden:
						glfwSetInputMode(inout_window.m_context, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
						break;
					default:
						break;
					}

					needs_cursor_reset = true;
				}
			);
		}
	);
}

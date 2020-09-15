#include "sic/system_window.h"

#include "sic/asset_types.h"
#include "sic/system_view.h"
#include "sic/gl_includes.h"
#include "sic/system_model.h"
#include "sic/component_transform.h"
#include "sic/shader_parser.h"
#include "sic/system_ui.h"
#include "sic/state_render_scene.h"

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
		}

		static void window_scrolled(GLFWwindow* in_window, double in_xoffset, double in_yoffset)
		{
			Window_proxy* wdw = static_cast<Window_proxy*>(glfwGetWindowUserPointer(in_window));

			if (!wdw)
				return;

			wdw->m_scroll_offset_x += in_xoffset;
			wdw->m_scroll_offset_y += in_yoffset;
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
							inout_ui_state.update
							(
								[name, id, window_dimensions](Ui_context& inout_ui_context)
								{
									Ui_widget_canvas* canvas = inout_ui_context.find<Ui_widget_canvas>(name);
									assert(canvas);

									canvas->m_reference_dimensions = window_dimensions;
									inout_ui_context.set_window_info(name, window_dimensions, id);
								}
							);
						}
					);
				}
			}
		}
	}

	for (Render_object_window& window : scene_state.m_windows.m_objects)
	{
		if (glfwWindowShouldClose(window.m_context) != 0)
		{
			const Render_object_window* main_window = scene_state.m_windows.find_object(window_state.m_main_window_interface->m_window_id);
			
			if (main_window == &window)
			{
				this_thread().update_deferred([](Engine_context in_context) { in_context.shutdown(); });
			}
			else
			{
				auto to_destroy_it = window_state.m_window_name_to_interfaces_lut.find(window.m_name);

				if (to_destroy_it != window_state.m_window_name_to_interfaces_lut.end())
					window_state.destroy_window(in_processor, to_destroy_it->first);
			}
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

sic::Window_proxy& sic::State_window::create_window(Processor_window in_processor, const std::string& in_name, const glm::ivec2& in_dimensions)
{
	std::scoped_lock lock(m_mutex);

	State_render_scene& scene_state = in_processor.get_state_checked_w<State_render_scene>();

	auto& window_interface_ptr = m_window_name_to_interfaces_lut[in_name];

	if (window_interface_ptr.get())
		return *window_interface_ptr;

	window_interface_ptr = std::make_unique<Window_proxy>();
	auto window_interface_ptr_raw = window_interface_ptr.get();

	if (m_main_window_interface == nullptr)
		m_main_window_interface = window_interface_ptr_raw;

	window_interface_ptr_raw->m_name = in_name;
	window_interface_ptr_raw->m_dimensions = in_dimensions;
	window_interface_ptr_raw->m_window_id = scene_state.m_windows.create_object
	(
		[window_interface_ptr_raw, in_name, in_dimensions, resource_context = m_resource_context](Render_object_window& in_out_window)
		{
			glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
			glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL
			glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
			//glfwWindowHint(GLFW_DECORATED, 0);

			in_out_window.m_context = glfwCreateWindow(in_dimensions.x, in_dimensions.y, in_name.c_str(), NULL, resource_context);
			in_out_window.m_name = in_name;

			if (in_out_window.m_context == NULL)
			{
				SIC_LOG_E(g_log_renderer, "Failed to open GLFW window. GPU not 3.3 compatible.");
				glfwTerminate();
				return;
			}

			glfwSetWindowUserPointer(in_out_window.m_context, window_interface_ptr_raw);
			glfwSetWindowFocusCallback(in_out_window.m_context, &System_window_functions::window_focused);
			glfwSetScrollCallback(in_out_window.m_context, &System_window_functions::window_scrolled);

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
			in_out_window.m_render_target.emplace(in_dimensions, OpenGl_texture_format::rgb,  false);
			in_out_window.m_ui_render_target.emplace(in_dimensions * 2, OpenGl_texture_format::rgba, false);
		}
	);

	in_processor.update_state_deferred<State_ui>
	(
		[in_name, in_dimensions, id = window_interface_ptr_raw->m_window_id](State_ui& inout_ui_state)
		{
			inout_ui_state.update
			(
				[in_name, in_dimensions, id](Ui_context& inout_ui_context)
				{
					Ui_widget_canvas& canvas = inout_ui_context.create<Ui_widget_canvas>(in_name);
					canvas.m_reference_dimensions = in_dimensions;

					inout_ui_context.set_window_info(in_name, in_dimensions, id);
				}
			);
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
	scene_state.m_windows.destroy_object(window_interface_ptr->second->m_window_id);

	in_processor.update_state_deferred<State_ui>
	(
		[name = window_interface_ptr->second->get_name()](State_ui& inout_ui_state)
		{
			inout_ui_state.update
			(
				[name](Ui_context& inout_ui_context)
				{
					inout_ui_context.destroy(name);
				}
			);
		}
	);

	m_window_name_to_interfaces_lut.erase(window_interface_ptr);
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

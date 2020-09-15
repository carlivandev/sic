#include "sic/system_input.h"

#include "sic/system_window.h"
#include "sic/component_view.h"

#include "sic/gl_includes.h"

#include "sic/core/logger.h"

void sic::System_input::on_created(Engine_context in_context)
{
	in_context.register_state<State_input>("input");
}

void sic::System_input::on_engine_tick(Engine_context in_context, float in_time_delta) const
{
	in_time_delta;
	in_context.schedule(update_input);
}

void sic::System_input::update_input(Processor<Processor_flag_write<State_input>, Processor_flag_write<State_window>, Processor_flag_read<State_render_scene>> in_processor)
{
	State_input& input_state = in_processor.get_state_checked_w<State_input>();
	State_window& window_state = in_processor.get_state_checked_w<State_window>();
	const State_render_scene& scene_state = in_processor.get_state_checked_r<State_render_scene>();

	input_state.m_scroll_offset_x = 0.0f;
	input_state.m_scroll_offset_y = 0.0f;

	Window_proxy* focused_window = window_state.get_focused_window();

	if (!focused_window)
		return;

	const Render_object_window* window_ro = scene_state.m_windows.find_object(focused_window->m_window_id);

	if (!window_ro)
		return;

	input_state.m_scroll_offset_x += focused_window->m_scroll_offset_x;
	input_state.m_scroll_offset_y += focused_window->m_scroll_offset_y;

	focused_window->m_scroll_offset_x = 0.0;
	focused_window->m_scroll_offset_y = 0.0;
	
	const i32 first_valid_key = 32;

	for (i32 i = first_valid_key; i < GLFW_KEY_LAST; i++)
		input_state.key_last_frame_down[i] = input_state.key_this_frame_down[i];

	for (i32 i = first_valid_key; i < GLFW_KEY_LAST; i++)
		input_state.key_this_frame_down[i] = glfwGetKey(window_ro->m_context, i) == GLFW_PRESS;

	for (i32 i = 0; i < GLFW_MOUSE_BUTTON_LAST; i++)
		input_state.mouse_last_frame_down[i] = input_state.mouse_this_frame_down[i];

	for (i32 i = 0; i < GLFW_MOUSE_BUTTON_LAST; i++)
		input_state.mouse_this_frame_down[i] = glfwGetMouseButton(window_ro->m_context, i) == GLFW_PRESS;
}

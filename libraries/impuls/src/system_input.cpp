#include "impuls/system_input.h"

#include "impuls/system_window.h"
#include "impuls/view.h"

#include "impuls/gl_includes.h"

void impuls::system_input::on_created(world_context&& in_context) const
{
	in_context.register_state<state_input>("input");
}

void impuls::system_input::on_tick(world_context&& in_context, float in_time_delta) const
{
	in_time_delta;

	state_input* input_state = in_context.get_state<state_input>();

	if (!input_state)
		return;

	input_state->m_scroll_offset_x = 0.0f;
	input_state->m_scroll_offset_y = 0.0f;

	for (component_view& component_view_it : in_context.components<component_view>())
	{
		if (!component_view_it.m_window_render_on)
			continue;

		component_window& cur_window = component_view_it.m_window_render_on->get<component_window>();

		if (!cur_window.m_is_focused)
			continue;

		input_state->m_scroll_offset_x += cur_window.m_scroll_offset_x;
		input_state->m_scroll_offset_y += cur_window.m_scroll_offset_y;

		cur_window.m_scroll_offset_x = 0.0;
		cur_window.m_scroll_offset_y = 0.0;

		const i32 first_valid_key = 32;

		for (i32 i = first_valid_key; i < GLFW_KEY_LAST; i++)
			input_state->key_last_frame_down[i] = input_state->key_this_frame_down[i];

		for (i32 i = first_valid_key; i < GLFW_KEY_LAST; i++)
			input_state->key_this_frame_down[i] = glfwGetKey(cur_window.m_window, i) == GLFW_PRESS;

		for (i32 i = 0; i < GLFW_MOUSE_BUTTON_LAST; i++)
			input_state->mouse_last_frame_down[i] = input_state->mouse_this_frame_down[i];

		for (i32 i = 0; i < GLFW_MOUSE_BUTTON_LAST; i++)
			input_state->mouse_this_frame_down[i] = glfwGetMouseButton(cur_window.m_window, i) == GLFW_PRESS;

	}
}
#include "sic/system_input.h"

#include "sic/system_window.h"
#include "sic/component_view.h"

#include "sic/gl_includes.h"

void sic::System_input::on_created(Engine_context&& in_context)
{
	in_context.register_state<State_input>("input");
}

void sic::System_input::on_engine_tick(Engine_context&& in_context, float in_time_delta) const
{
	in_time_delta;

	State_input* input_state = in_context.get_state<State_input>();

	if (!input_state)
		return;

	input_state->m_scroll_offset_x = 0.0f;
	input_state->m_scroll_offset_y = 0.0f;

	for (auto& level : in_context.m_engine.m_levels)
	{
		level->for_each<Component_view>
		(
			[input_state](Component_view& component_view_it)
			{
				if (!component_view_it.get_window())
					return;

				Component_window& cur_window = component_view_it.get_window()->get<Component_window>();

				if (!cur_window.m_is_focused)
					return;

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
		);
	}
}
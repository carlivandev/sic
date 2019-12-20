#include "impuls/system_editor_view_controller.h"

#include "impuls/gl_includes.h"
#include "impuls/system_window.h"
#include "impuls/component_view.h"
#include "impuls/logger.h"

#include "glm/gtc/matrix_transform.hpp"


void sic::System_editor_view_controller::on_created(Engine_context&& in_context)
{
	in_context.register_component_type<Component_editor_view_controller>("evcd", 1);
	in_context.register_object<Object_editor_view_controller>("evc", 1);
}

void sic::System_editor_view_controller::on_begin_simulation(Level_context&& in_context) const
{
	in_context.for_each<Component_editor_view_controller>
	(
		[&in_context](auto& evc)
		{
			if (!evc.m_view_to_control)
				return;

			if (!evc.m_view_to_control->get_window())
				return;

			Component_window& wd = evc.m_view_to_control->get_window()->get<Component_window>();

			sic::i32 window_width, window_height;
			glfwGetWindowSize(wd.m_window, &window_width, &window_height);
		}
	);
}

void sic::System_editor_view_controller::on_tick(Level_context&& in_context, float in_time_delta) const
{
	State_input* input_state = in_context.m_engine.get_state<State_input>();

	if (!input_state)
		return;

	if (input_state->is_key_pressed(Key::escape))
		in_context.m_engine.shutdown();

	in_context.for_each<Component_editor_view_controller>
	(
		[input_state, in_time_delta](Component_editor_view_controller& evc)
		{
			if (!evc.m_view_to_control)
				return;

			if (!evc.m_view_to_control->get_window())
				return;

			Object_window& window_to_control = *evc.m_view_to_control->get_window();
			Component_window& wd = window_to_control.get<Component_window>();

			if (input_state->is_mousebutton_pressed(Mousebutton::right))
				wd.set_input_mode(e_window_input_mode::disabled);
			else if (input_state->is_mousebutton_released(Mousebutton::right))
				wd.set_input_mode(e_window_input_mode::normal);

			if (!input_state->is_mousebutton_down(Mousebutton::right))
				return;

			const float to_increment_with = evc.m_speed_multiplier_incrementation * static_cast<float>(input_state->m_scroll_offset_y);
			evc.m_speed_multiplier = glm::clamp(evc.m_speed_multiplier + to_increment_with, evc.m_speed_multiplier_min, evc.m_speed_multiplier_max);

			// Compute new orientation
			const float horizontal_angle = evc.m_mouse_speed * in_time_delta * wd.get_cursor_movement().x;
			float vertical_angle = evc.m_mouse_speed * in_time_delta * -wd.get_cursor_movement().y;

			Component_transform* trans = evc.m_view_to_control->owner().find<Component_transform>();

			const float max_vertical_angle = 80.0f;

			evc.m_pitch = glm::clamp(evc.m_pitch + vertical_angle, -max_vertical_angle, max_vertical_angle);
			evc.m_yaw += horizontal_angle;

			glm::vec3 front;
			front.x = cos(glm::radians(evc.m_yaw)) * cos(glm::radians(evc.m_pitch));
			front.y = sin(glm::radians(evc.m_pitch));
			front.z = sin(glm::radians(evc.m_yaw)) * cos(glm::radians(evc.m_pitch));

			trans->look_at(front, Transform::up);

			const glm::vec3 direction = trans->get_forward();
			const glm::vec3 right = trans->get_right();
			const glm::vec3 up = Transform::up;

			const float move_speed = in_time_delta * evc.m_speed * evc.m_speed_multiplier;

			glm::vec3 translation = { 0.0f, 0.0f, 0.0f };

			if (input_state->is_key_down(Key::w))
				translation += direction * move_speed;

			if (input_state->is_key_down(Key::s))
				translation -= direction * move_speed;

			if (input_state->is_key_down(Key::d))
				translation += right * move_speed;

			if (input_state->is_key_down(Key::a))
				translation -= right * move_speed;

			if (input_state->is_key_down(Key::space))
				translation += up * move_speed;

			if (input_state->is_key_down(Key::left_control))
				translation -= up * move_speed;

			trans->translate(translation);
		}
	);
}
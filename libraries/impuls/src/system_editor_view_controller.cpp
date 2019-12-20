#include "impuls/system_editor_view_controller.h"

#include "impuls/gl_includes.h"
#include "impuls/system_window.h"
#include "impuls/component_view.h"
#include "impuls/logger.h"

#include "glm/gtc/matrix_transform.hpp"


void impuls::system_editor_view_controller::on_created(engine_context&& in_context)
{
	in_context.register_component_type<component_editor_view_controller>("evcd", 1);
	in_context.register_object<object_editor_view_controller>("evc", 1);
}

void impuls::system_editor_view_controller::on_begin_simulation(level_context&& in_context) const
{
	in_context.for_each<component_editor_view_controller>
	(
		[&in_context](auto& evc)
		{
			if (!evc.m_view_to_control)
				return;

			if (!evc.m_view_to_control->get_window())
				return;

			component_window& wd = evc.m_view_to_control->get_window()->get<component_window>();

			impuls::i32 window_width, window_height;
			glfwGetWindowSize(wd.m_window, &window_width, &window_height);
		}
	);
}

void impuls::system_editor_view_controller::on_tick(level_context&& in_context, float in_time_delta) const
{
	state_input* input_state = in_context.m_engine.get_state<state_input>();

	if (!input_state)
		return;

	if (input_state->is_key_pressed(e_key::escape))
		in_context.m_engine.shutdown();

	in_context.for_each<component_editor_view_controller>
	(
		[input_state, in_time_delta](component_editor_view_controller& evc)
		{
			if (!evc.m_view_to_control)
				return;

			if (!evc.m_view_to_control->get_window())
				return;

			object_window& window_to_control = *evc.m_view_to_control->get_window();
			component_window& wd = window_to_control.get<component_window>();

			if (input_state->is_mousebutton_pressed(e_mousebutton::right))
				wd.set_input_mode(e_window_input_mode::disabled);
			else if (input_state->is_mousebutton_released(e_mousebutton::right))
				wd.set_input_mode(e_window_input_mode::normal);

			if (!input_state->is_mousebutton_down(e_mousebutton::right))
				return;

			const float to_increment_with = evc.m_speed_multiplier_incrementation * static_cast<float>(input_state->m_scroll_offset_y);
			evc.m_speed_multiplier = glm::clamp(evc.m_speed_multiplier + to_increment_with, evc.m_speed_multiplier_min, evc.m_speed_multiplier_max);

			// Compute new orientation
			const float horizontal_angle = evc.m_mouse_speed * in_time_delta * wd.get_cursor_movement().x;
			float vertical_angle = evc.m_mouse_speed * in_time_delta * -wd.get_cursor_movement().y;

			component_transform* trans = evc.m_view_to_control->owner().find<component_transform>();

			const float max_vertical_angle = 80.0f;

			evc.m_pitch = glm::clamp(evc.m_pitch + vertical_angle, -max_vertical_angle, max_vertical_angle);
			evc.m_yaw += horizontal_angle;

			glm::vec3 front;
			front.x = cos(glm::radians(evc.m_yaw)) * cos(glm::radians(evc.m_pitch));
			front.y = sin(glm::radians(evc.m_pitch));
			front.z = sin(glm::radians(evc.m_yaw)) * cos(glm::radians(evc.m_pitch));

			trans->look_at(front, transform::up);

			const glm::vec3 direction = trans->get_forward();
			const glm::vec3 right = trans->get_right();
			const glm::vec3 up = transform::up;

			const float move_speed = in_time_delta * evc.m_speed * evc.m_speed_multiplier;

			glm::vec3 translation = { 0.0f, 0.0f, 0.0f };

			if (input_state->is_key_down(e_key::w))
				translation += direction * move_speed;

			if (input_state->is_key_down(e_key::s))
				translation -= direction * move_speed;

			if (input_state->is_key_down(e_key::d))
				translation += right * move_speed;

			if (input_state->is_key_down(e_key::a))
				translation -= right * move_speed;

			if (input_state->is_key_down(e_key::space))
				translation += up * move_speed;

			if (input_state->is_key_down(e_key::left_control))
				translation -= up * move_speed;

			trans->translate(translation);
		}
	);
}
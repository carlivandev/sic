#include "impuls/system_editor_view_controller.h"

#include "impuls/gl_includes.h"
#include "impuls/system_window.h"
#include "impuls/view.h"

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

			if (!evc.m_view_to_control->m_window_render_on)
				return;

			component_window& wd = evc.m_view_to_control->m_window_render_on->get<component_window>();

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
		in_context.m_engine.destroy();

	in_context.for_each<component_editor_view_controller>
	(
		[input_state, in_time_delta](component_editor_view_controller& evc)
		{
			if (!evc.m_view_to_control)
				return;

			if (!evc.m_view_to_control->m_window_render_on)
				return;

			object_window& window_to_control = *evc.m_view_to_control->m_window_render_on;
			component_window& wd = window_to_control.get<component_window>();

			if (input_state->is_mousebutton_pressed(e_mousebutton::right))
				wd.set_input_mode(e_window_input_mode::disabled);
			else if (input_state->is_mousebutton_released(e_mousebutton::right))
				wd.set_input_mode(e_window_input_mode::normal);

			if (!input_state->is_mousebutton_down(e_mousebutton::right))
				return;

			const float to_increment_with = evc.m_speed_multiplier_incrementation * static_cast<float>(input_state->m_scroll_offset_y);
			evc.m_speed_multiplier = glm::clamp(evc.m_speed_multiplier + to_increment_with, evc.m_speed_multiplier_min, evc.m_speed_multiplier_max);

			glm::vec3& position = evc.m_view_to_control->m_position;
			float& horizontal_angle = evc.m_view_to_control->m_horizontal_angle;
			float& vertical_angle = evc.m_view_to_control->m_vertical_angle;

			// Compute new orientation
			horizontal_angle += evc.m_mouse_speed * in_time_delta * -wd.get_cursor_movement().x;
			vertical_angle += evc.m_mouse_speed * in_time_delta * -wd.get_cursor_movement().y;

			const float max_vertical_angle = glm::radians(80.0f);
			vertical_angle = glm::clamp(vertical_angle, -max_vertical_angle, max_vertical_angle);

			// Direction : Spherical coordinates to Cartesian coordinates conversion
			const glm::vec3 direction
			(
				cos(vertical_angle) * sin(horizontal_angle),
				sin(vertical_angle),
				cos(vertical_angle) * cos(horizontal_angle)
			);

			// Right vector
			glm::vec3 right = glm::vec3
			(
				sin(horizontal_angle - 3.14f / 2.0f),
				0,
				cos(horizontal_angle - 3.14f / 2.0f)
			);

			// Up vector : perpendicular to both direction and right
			glm::vec3 up = glm::cross(right, direction);

			const float move_speed = in_time_delta * evc.m_speed * evc.m_speed_multiplier;

			if (input_state->is_key_down(e_key::w))
				position += direction * move_speed;

			if (input_state->is_key_down(e_key::s))
				position -= direction * move_speed;

			if (input_state->is_key_down(e_key::d))
				position += right * move_speed;

			if (input_state->is_key_down(e_key::a))
				position -= right * move_speed;

			if (input_state->is_key_down(e_key::space))
				position += up * move_speed;

			if (input_state->is_key_down(e_key::left_control))
				position -= up * move_speed;
		}
	);
}
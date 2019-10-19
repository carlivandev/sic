#include "impuls/system_editor_view_controller.h"

#include "impuls/world_context.h"
#include "impuls/gl_includes.h"
#include "impuls/system_window.h"
#include "impuls/view.h"

#include "glm/gtc/matrix_transform.hpp"


void impuls::system_editor_view_controller::on_created(world_context&& in_context) const
{
	in_context.register_component_type<component_editor_view_controller>("evcd", 1, 1);
	in_context.register_object<object_editor_view_controller>("evc", 1, 1);
}

void impuls::system_editor_view_controller::on_begin_simulation(world_context&& in_context) const
{
	for (auto& evc : in_context.each<component_editor_view_controller>())
	{
		if (!evc.m_view_to_control)
			continue;

		if (!evc.m_view_to_control->m_window_render_on)
			continue;

		component_window& wd = evc.m_view_to_control->m_window_render_on->get<component_window>();

		impuls::i32 window_width, window_height;
		glfwGetWindowSize(wd.m_window, &window_width, &window_height);

		// reset mouse position for next frame
		glfwSetCursorPos(wd.m_window, window_width / 2, window_height / 2);
	}
}

void impuls::system_editor_view_controller::on_tick(world_context&& in_context, float in_time_delta) const
{
	state_input* input_state = in_context.get_state<state_input>();

	if (!input_state)
		return;

	if (input_state->is_key_pressed(e_key::escape))
		in_context.m_world->destroy();

	for (auto& evc : in_context.each<component_editor_view_controller>())
	{
		if (!evc.m_view_to_control)
			continue;

		if (!evc.m_view_to_control->m_window_render_on)
			continue;

		object_window& window_to_control = *evc.m_view_to_control->m_window_render_on;
		component_window& wd = window_to_control.get<component_window>();

		double xpos, ypos;
		glfwGetCursorPos(wd.m_window, &xpos, &ypos);
		
		if (input_state->is_mousebutton_pressed(e_mousebutton::right))
			evc.m_mouse_pos_on_rightclick = { xpos, ypos };
		else if (input_state->is_mousebutton_pressed(e_mousebutton::right))
			glfwSetCursorPos(wd.m_window, evc.m_mouse_pos_on_rightclick.x, evc.m_mouse_pos_on_rightclick.y);

		if (!input_state->is_mousebutton_down(e_mousebutton::right))
			continue;

		const float to_increment_with = evc.m_speed_multiplier_incrementation * static_cast<float>(input_state->m_scroll_offset_y);
		evc.m_speed_multiplier = glm::clamp(evc.m_speed_multiplier + to_increment_with, evc.m_speed_multiplier_min, evc.m_speed_multiplier_max);

		glm::vec3& position = evc.m_view_to_control->m_position;
		float& horizontal_angle = evc.m_view_to_control->m_horizontal_angle;
		float& vertical_angle = evc.m_view_to_control->m_vertical_angle;

		// Compute new orientation
		horizontal_angle += evc.m_mouse_speed * in_time_delta * float(evc.m_mouse_pos_on_rightclick.x - xpos);
		vertical_angle += evc.m_mouse_speed * in_time_delta * float(evc.m_mouse_pos_on_rightclick.y - ypos);

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

		// reset mouse position for next frame
		glfwSetCursorPos(wd.m_window, evc.m_mouse_pos_on_rightclick.x, evc.m_mouse_pos_on_rightclick.y);
	}
}
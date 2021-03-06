#include "sic/system_editor_view_controller.h"

#include "sic/gl_includes.h"
#include "sic/component_view.h"

#include "sic/core/logger.h"

#include "glm/gtc/matrix_transform.hpp"


void sic::System_editor_view_controller::on_created(Engine_context in_context)
{
	in_context.register_component<Component_editor_view_controller>("evcd", 1);
	in_context.register_object<Object_editor_view_controller>("evc", 1);
}

void sic::System_editor_view_controller::on_tick(Scene_context in_context, float in_time_delta) const
{
	in_time_delta;
	in_context.schedule(update_controllers);
}

void sic::System_editor_view_controller::update_controllers(Scene_processor<Processor_flag_write<Component_editor_view_controller>, Processor_flag_write_single<Component_transform>, Processor_flag_read<State_input>> in_processor)
{
	const State_input& input_state = in_processor.get_state_checked_r<State_input>();
	in_processor.for_each
	(
		[in_processor, time_delta = in_processor.get_time_delta(), &input_state](Component_editor_view_controller& evc) mutable
		{
			if (!evc.m_view_to_control)
				return;

			Window_proxy* wd = evc.m_view_to_control->get_window();

			if (!wd)
				return;

			if (!wd->m_is_focused)
				return;

			if (input_state.is_mousebutton_pressed(Mousebutton::right))
				wd->set_input_mode(in_processor.get_engine_processor(), Window_input_mode::disabled);
			else if (input_state.is_mousebutton_released(Mousebutton::right))
				wd->set_input_mode(in_processor.get_engine_processor(), Window_input_mode::normal);

			if (!input_state.is_mousebutton_down(Mousebutton::right))
				return;

			const float to_increment_with = evc.m_speed_multiplier_incrementation * static_cast<float>(input_state.m_scroll_offset_y);
			evc.m_speed_multiplier = glm::clamp(evc.m_speed_multiplier + to_increment_with, evc.m_speed_multiplier_min, evc.m_speed_multiplier_max);

			// Compute new orientation
			const float horizontal_angle = evc.m_mouse_speed * wd->get_cursor_movement().x;
			float vertical_angle = evc.m_mouse_speed * -wd->get_cursor_movement().y;

			Component_transform* trans = in_processor.find_w<Component_transform>(evc.m_view_to_control->get_owner());

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

			const float move_speed = time_delta * evc.m_speed * evc.m_speed_multiplier;

			glm::vec3 translation = { 0.0f, 0.0f, 0.0f };

			if (input_state.is_key_down(Key::w))
				translation += direction * move_speed;

			if (input_state.is_key_down(Key::s))
				translation -= direction * move_speed;

			if (input_state.is_key_down(Key::d))
				translation += right * move_speed;

			if (input_state.is_key_down(Key::a))
				translation -= right * move_speed;

			if (input_state.is_key_down(Key::space))
				translation += up * move_speed;

			if (input_state.is_key_down(Key::left_control))
				translation -= up * move_speed;

			trans->translate(translation);
		}
	);
}
#pragma once
#include "sic/system_window.h"
#include "sic/state_render_scene.h"

#include "sic/core/system.h"
#include "sic/core/scene_context.h"
#include "sic/core/object.h"

#include "glm/vec2.hpp"

namespace sic
{
	struct Component_view;
	struct State_input;
	struct Component_transform;

	struct Component_editor_view_controller : public Component
	{
		Component_view* m_view_to_control = nullptr;

		float m_speed = 100.0f; // units / second
		float m_mouse_speed = 0.1f;

		float m_speed_multiplier = 1.0f;
		float m_speed_multiplier_max = 20.0f;
		float m_speed_multiplier_min = 0.5f;
		float m_speed_multiplier_incrementation = 0.25f;

		float m_pitch = 0.0f;
		float m_yaw = -90.0f;
	};

	struct Object_editor_view_controller : public Object<Object_editor_view_controller, Component_editor_view_controller> {};

	struct System_editor_view_controller : System
	{
		virtual void on_created(Engine_context in_context) override;
		virtual void on_tick(Scene_context in_context, float in_time_delta) const override;

		static void update_controllers(Scene_processor<Processor_flag_write<Component_editor_view_controller>, Processor_flag_write_single<Component_transform>, Processor_flag_read<State_input>> in_processor);
	};
}
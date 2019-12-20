#pragma once
#include "impuls/system.h"
#include "impuls/engine_context.h"
#include "impuls/object.h"
#include "glm/vec2.hpp"

namespace impuls
{
	struct Component_view;

	struct Component_editor_view_controller : public Component
	{
		Component_view* m_view_to_control = nullptr;

		float m_speed = 3.0f; // 3 units / second
		float m_mouse_speed = 3.0f;

		float m_speed_multiplier = 1.0f;
		float m_speed_multiplier_max = 10.0f;
		float m_speed_multiplier_min = 0.5f;
		float m_speed_multiplier_incrementation = 0.25f;

		float m_pitch = 0.0f;
		float m_yaw = -90.0f;
	};

	struct Object_editor_view_controller : public Object<Object_editor_view_controller, Component_editor_view_controller> {};

	struct System_editor_view_controller : System
	{
		virtual void on_created(Engine_context&& in_context) override;
		virtual void on_begin_simulation(Level_context&& in_context) const override;
		virtual void on_tick(Level_context&& in_context, float in_time_delta) const override;
	};
}
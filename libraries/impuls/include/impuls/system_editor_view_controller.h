#pragma once
#include "system.h"
#include "world_context.h"
#include "glm/vec2.hpp"

namespace impuls
{
	struct component_view;

	struct component_editor_view_controller : public i_component
	{
		component_view* m_view_to_control = nullptr;

		float m_speed = 3.0f; // 3 units / second
		float m_mouse_speed = 0.05f;

		float m_speed_multiplier = 1.0f;
		float m_speed_multiplier_max = 10.0f;
		float m_speed_multiplier_min = 0.5f;
		float m_speed_multiplier_incrementation = 0.25f;
	};

	struct object_editor_view_controller : public i_object<component_editor_view_controller> {};

	struct system_editor_view_controller : i_system
	{
		virtual void on_created(world_context&& in_context) const override;
		virtual void on_begin_simulation(world_context&& in_context) const override;
		virtual void on_tick(world_context&& in_context, float in_time_delta) const override;
	};
}
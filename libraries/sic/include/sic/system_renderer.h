#pragma once
#include "sic/system.h"
#include "sic/state_render_scene.h"

#include <unordered_map>

namespace sic
{
	struct State_debug_drawing : State
	{
		std::unordered_map<i32, Render_object_id<Render_object_debug_drawer>> m_level_id_to_debug_drawer_ids;
	};

	struct System_renderer : System
	{
		virtual void on_created(Engine_context in_context) override;
		virtual void on_engine_tick(Engine_context in_context, float in_time_delta) const override;
	};
}
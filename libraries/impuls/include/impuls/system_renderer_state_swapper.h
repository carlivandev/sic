#pragma once
#include "system.h"
#include "world_context.h"

namespace impuls
{
	struct system_renderer_state_swapper : i_system
	{
		virtual void on_tick(world_context&& in_context, float in_time_delta) const override;
	};
}
#pragma once
#include "system.h"
#include "engine_context.h"

namespace sic
{
	struct System_renderer_state_swapper : System
	{
		virtual void on_engine_tick(Engine_context&& in_context, float in_time_delta) const override;
	};
}
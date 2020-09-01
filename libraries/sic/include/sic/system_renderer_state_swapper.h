#pragma once
#include "sic/core/system.h"
#include "sic/core/engine_context.h"

namespace sic
{
	struct System_renderer_state_swapper : System
	{
		virtual void on_engine_tick(Engine_context in_context, float in_time_delta) const override;
	};
}
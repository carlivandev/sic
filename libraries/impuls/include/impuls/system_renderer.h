#pragma once
#include "system.h"

namespace impuls
{
	struct system_renderer : i_system
	{
		virtual void on_created(engine_context&& in_context) override;
		virtual void on_engine_tick(engine_context&& in_context, float in_time_delta) const override;
	};
}
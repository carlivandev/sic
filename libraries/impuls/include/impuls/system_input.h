#pragma once
#include "system.h"
#include "world_context.h"
#include "input.h"

namespace impuls
{
	struct system_input : i_system
	{
		virtual void on_created(world_context&& in_context) const override;
		virtual void on_tick(world_context&& in_context, float in_time_delta) const override;
	};
}
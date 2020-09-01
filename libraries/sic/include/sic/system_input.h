#pragma once
#include "sic/input.h"

#include "sic/core/system.h"
#include "sic/core/engine_context.h"

namespace sic
{
	struct System_input : System
	{
		virtual void on_created(Engine_context in_context) override;
		virtual void on_engine_tick(Engine_context in_context, float in_time_delta) const override;
	};
}
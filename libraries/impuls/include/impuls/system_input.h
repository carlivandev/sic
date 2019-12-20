#pragma once
#include "system.h"
#include "engine_context.h"
#include "input.h"

namespace sic
{
	struct System_input : System
	{
		virtual void on_created(Engine_context&& in_context) override;
		virtual void on_engine_tick(Engine_context&& in_context, float in_time_delta) const override;
	};
}
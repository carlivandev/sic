#pragma once
#include "system.h"

namespace sic
{
	struct System_renderer : System
	{
		virtual void on_created(Engine_context&& in_context) override;
		virtual void on_engine_tick(Engine_context&& in_context, float in_time_delta) const override;
	};
}
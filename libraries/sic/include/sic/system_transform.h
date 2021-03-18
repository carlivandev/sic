#pragma once
#include "sic/core/system.h"

#include "sic/component_transform.h"
#include "sic/core/engine_context.h"

namespace sic
{
	struct System_transform : System
	{
		virtual void on_created(Engine_context in_context) override;
	};
}
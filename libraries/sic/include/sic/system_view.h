#pragma once
#include "sic/component_view.h"
#include "sic/component_transform.h"

#include "sic/core/system.h"

namespace sic
{
	struct System_view : System
	{
		virtual void on_created(Engine_context in_context) override;
	};
}
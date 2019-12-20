#pragma once
#include "impuls/system.h"

#include "impuls/component_view.h"
#include "impuls/component_transform.h"

namespace impuls
{
	struct system_view : i_system
	{
		virtual void on_created(engine_context&& in_context) override;
	};
}
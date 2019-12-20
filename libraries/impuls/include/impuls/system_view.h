#pragma once
#include "impuls/system.h"

#include "impuls/component_view.h"
#include "impuls/component_transform.h"

namespace impuls
{
	struct System_view : System
	{
		virtual void on_created(Engine_context&& in_context) override;
	};
}
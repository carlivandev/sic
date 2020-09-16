#pragma once
#include "sic/input.h"

#include "sic/core/system.h"
#include "sic/core/engine_context.h"
#include "sic/core/scene_context.h"

namespace sic
{
	struct State_window;
	struct State_render_scene;

	struct System_input : System
	{
		virtual void on_created(Engine_context in_context) override;
		virtual void on_engine_tick(Engine_context in_context, float in_time_delta) const override;

		static void update_input(Processor<Processor_flag_write<State_input>, Processor_flag_write<State_window>, Processor_flag_read<State_render_scene>> in_processor);
	};
}
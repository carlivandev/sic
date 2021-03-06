#pragma once
#include "sic/core/system.h"

#include "sic/system_window.h"
#include "sic/asset_material.h"

namespace sic::editor
{
	struct State_editor : State
	{
		
	};

	struct System_editor : System
	{
		void on_created(Engine_context in_context) override;
		void on_engine_finalized(Engine_context in_context) const override;
	};

	using Processor_editor_window = Engine_processor<Processor_window, Processor_flag_read<State_editor>>;

	void create_editor_window(Processor_editor_window in_processor, const std::string& in_name, const glm::ivec2& in_dimensions);
}
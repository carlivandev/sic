#pragma once
#include "sic/core/system.h"

#include "sic/system_window.h"
#include "sic/asset_material.h"

namespace sic::editor
{
	struct State_editor : State
	{
		Asset_ref<Asset_material> m_minimize_icon_material;
		Asset_ref<Asset_material> m_maximize_icon_material;
		Asset_ref<Asset_material> m_close_icon_material;

		Asset_ref<Asset_material> m_toolbar_material;
		Asset_ref<Asset_material> m_sic_icon_material;
	};

	struct System_editor : System
	{
		void on_created(Engine_context in_context) override;
		void on_engine_finalized(Engine_context in_context) const override;
	};

	using Processor_editor_window = Processor<Processor_window, Processor_flag_read<State_editor>, Processor_flag_deferred_write<State_ui>>;

	void create_editor_window(Processor_editor_window in_processor, const std::string& in_name, const glm::ivec2& in_dimensions);
}
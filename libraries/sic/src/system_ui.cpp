#include "sic/system_ui.h"

void sic::Ui_widget::calculate_render_transform()
{
	m_render_translation = { 0.0f, 0.0f };
	m_render_size = { 1.0f, 1.0f };
	m_render_rotation = 0.0f;

	if (m_parent)
	{
		m_parent->calculate_render_transform_for_slot_base(*this, m_render_translation, m_render_size, m_render_rotation);

		m_render_translation += m_local_translation;
		m_render_size *= m_local_scale;
		m_render_rotation += m_local_rotation;
	}
	else
	{
		/*
		if a widget doesnt have a parent, its size should cover the entire screen (1.0f, 1.0f)
		otherwise, slot types should use their data to set the correct size

		(ofcourse, locals still apply)
		*/
		m_render_translation = glm::vec2(0.5f, 0.5f) + m_local_translation;
		m_render_size = m_local_scale;
		m_render_rotation = m_local_rotation;
	}
}

void sic::Ui_widget::make_dirty(std::vector<Ui_widget*>& out_dirty_widgets)
{
	out_dirty_widgets.push_back(this);
}

void sic::System_ui::on_created(Engine_context in_context)
{
	in_context.register_state<State_ui>("State_ui");
}

void sic::System_ui::on_engine_tick(Engine_context in_context, float in_time_delta) const
{
	State_ui& ui_state = in_context.get_state_checked<State_ui>();
	State_render_scene& render_scene_state = in_context.get_state_checked<State_render_scene>();

	std::scoped_lock lock(ui_state.m_update_lock);

	for (auto& update : ui_state.m_updates)
	{
		update();

		for (Ui_widget* dirty_widget : ui_state.m_dirty_widgets)
			if (dirty_widget->m_key.has_value() && ui_state.m_widget_lut.find(dirty_widget->m_key.value()) == ui_state.m_widget_lut.end())
				ui_state.m_widget_lut.insert_or_assign(dirty_widget->m_key.value(), dirty_widget);
	}

	ui_state.m_updates.clear();

	std::vector<Asset_header*> asset_dependencies;

	for (Ui_widget* dirty_widget : ui_state.m_dirty_widgets)
	{
		asset_dependencies.clear();
		dirty_widget->gather_dependencies(asset_dependencies);

		for (Asset_header* header : asset_dependencies)
		{
			if (header->m_load_state != Asset_load_state::loaded)
			{
				//TODO: postpone render object update until loaded

				/*
				also, add setting in widget m_allow_asset_streaming

				default to false

				then, add get_has_finished_loading() to widget, which checks if itself and all children (recursively) this:
				return m_allow_asset_streaming || have dependencies.empty();
				*/
				
			}
		}

		if (dirty_widget->m_key.has_value() && ui_state.m_widget_lut.find(dirty_widget->m_key.value()) == ui_state.m_widget_lut.end())
			ui_state.m_widget_lut.insert_or_assign(dirty_widget->m_key.value(), dirty_widget);

		dirty_widget->calculate_render_transform();
		dirty_widget->update_render_scene(dirty_widget->m_render_translation, dirty_widget->m_render_size, dirty_widget->m_render_rotation, render_scene_state);
	}

	ui_state.m_dirty_widgets.clear();
}

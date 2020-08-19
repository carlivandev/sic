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

sic::Ui_widget* sic::Ui_widget::get_outermost_parent()
{
	if (m_parent)
		return m_parent->get_outermost_parent();

	return this;
}

void sic::Ui_widget::get_dependencies_not_loaded(std::vector<Asset_header*>& out_assets) const
{
	gather_dependencies(out_assets);

	std::remove_if(out_assets.begin(), out_assets.end(), [](Asset_header* in_header) { return in_header->m_load_state != Asset_load_state::loaded; });
}

bool sic::Ui_widget::get_ready_to_be_shown() const
{
	if (m_allow_dependency_streaming)
		return true;

	std::vector<Asset_header*> not_loaded_assets;
	get_dependencies_not_loaded(not_loaded_assets);

	return not_loaded_assets.empty();
}

void sic::Ui_widget::destroy(State_ui& inout_ui_state)
{
	inout_ui_state.m_widget_lut.erase(m_key.value());
	inout_ui_state.m_free_widget_indices.push_back(m_widget_index);
	inout_ui_state.m_widgets[m_widget_index].reset();
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

	Ui_context ui_context(ui_state);

	for (auto& update : ui_state.m_updates)
		update(ui_context);

	ui_state.m_updates.clear();

	std::vector<Asset_header*> asset_dependencies;

	for (Ui_widget* dirty_widget : ui_state.m_dirty_parent_widgets)
	{
		if (!dirty_widget->m_key.has_value())
			dirty_widget->m_key = xg::newGuid().str();

		if (ui_state.m_widget_lut.find(dirty_widget->m_key.value()) == ui_state.m_widget_lut.end())
			ui_state.m_widget_lut.insert_or_assign(dirty_widget->m_key.value(), dirty_widget);

		asset_dependencies.clear();
		dirty_widget->gather_dependencies(asset_dependencies);

		bool should_update_render_scene = true;

		for (Asset_header* header : asset_dependencies)
		{
			if (header->m_load_state != Asset_load_state::loaded)
			{
				//postpone render object update until loaded

				dirty_widget->m_dependencies_loaded_handle.set_callback
				(
					[&ui_state, key = dirty_widget->m_key.value()](Asset*)
					{
						auto it = ui_state.m_widget_lut.find(key);
						if (it == ui_state.m_widget_lut.end())
							return;

						ui_state.m_dirty_parent_widgets.insert(it->second->get_outermost_parent());
					}
				);

				header->m_on_loaded_delegate.try_bind(dirty_widget->m_dependencies_loaded_handle);

				should_update_render_scene = false;
				break;
				
			}
		}

		if (!should_update_render_scene)
			continue;

		dirty_widget->calculate_render_transform();
		dirty_widget->update_render_scene(dirty_widget->m_render_translation, dirty_widget->m_render_size, dirty_widget->m_render_rotation, render_scene_state);
	}

	ui_state.m_dirty_parent_widgets.clear();
}

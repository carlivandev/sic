#include "sic/system_ui.h"

sic::Log sic::g_log_ui("UI");
sic::Log sic::g_log_ui_verbose("UI", false);

void sic::Ui_widget::calculate_render_transform(const glm::vec2& in_window_size)
{
	in_window_size;

	if (!m_parent)
	{
		m_render_translation = { 0.0f, 0.0f };
		m_global_render_size = { 1.0f, 1.0f };
		m_render_rotation = 0.0f;
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

	auto it = std::remove_if(out_assets.begin(), out_assets.end(), [](Asset_header* in_header) { return in_header->m_load_state != Asset_load_state::loaded; });
	it;
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

sic::Interaction_consume sic::Ui_widget::on_cursor_move_over(const glm::vec2& in_cursor_pos, const glm::vec2& in_cursor_movement)
{
	SIC_LOG(g_log_ui_verbose, "on_cursor_move_over({0})", m_key.value());

	if (m_interaction_state == Ui_interaction_state::pressed || m_interaction_state == Ui_interaction_state::pressed_not_hovered)
		invoke<On_drag>(in_cursor_pos, in_cursor_movement);

	return m_interaction_consume_type;
}

sic::Interaction_consume sic::Ui_widget::on_hover_begin(const glm::vec2& in_cursor_pos)
{
	in_cursor_pos;
	SIC_LOG(g_log_ui_verbose, "on_hover_begin({0})", m_key.value());

	invoke<On_hover_begin>();
	return m_interaction_consume_type;
}

sic::Interaction_consume sic::Ui_widget::on_hover_end(const glm::vec2& in_cursor_pos)
{
	in_cursor_pos;
	SIC_LOG(g_log_ui_verbose, "on_hover_end({0})", m_key.value());

	invoke<On_hover_end>();
	return m_interaction_consume_type;
}

sic::Interaction_consume sic::Ui_widget::on_pressed(Mousebutton in_button, const glm::vec2& in_cursor_pos)
{
	in_button; in_cursor_pos;
	SIC_LOG(g_log_ui_verbose, "on_pressed({0})", m_key.value());
	
	invoke<On_pressed>();
	return m_interaction_consume_type;
}

sic::Interaction_consume sic::Ui_widget::on_released(Mousebutton in_button, const glm::vec2& in_cursor_pos)
{
	in_button; in_cursor_pos;
	SIC_LOG(g_log_ui_verbose, "on_released({0})", m_key.value());
	
	invoke<On_released>();
	return m_interaction_consume_type;
}

sic::Interaction_consume sic::Ui_widget::on_clicked(Mousebutton in_button, const glm::vec2& in_cursor_pos)
{
	in_button; in_cursor_pos;
	SIC_LOG(g_log_ui_verbose, "on_clicked({0})", m_key.value());
	
	invoke<On_clicked>();
	return m_interaction_consume_type;
}

void sic::System_ui::on_created(Engine_context in_context)
{
	in_context.register_state<State_ui>("State_ui");
}

void sic::System_ui::on_engine_tick(Engine_context in_context, float in_time_delta) const
{
	in_time_delta;
	in_context.schedule(update_ui);
}

void sic::System_ui::update_ui(Processor_ui in_processor)
{
	State_ui& ui_state = in_processor.get_state_checked_w<State_ui>();

	//update ui before drawing, using their previous render transforms (1 frame delay if they are modified)
	update_ui_interactions(in_processor);

	//TODO: two separate flags
	//redraw = only redraws the widget that requested the redraw
	//invalidate layout = recalculates the entire tree (maybe make a check that compares its previous render transform? so if no redraw was requested, it doesnt have to update the render scene, unless explicitly told so

	for (auto&& widget : ui_state.m_widgets)
	{
		if (widget && widget->m_redraw_requested)
		{
			ui_state.m_dirty_root_widgets.insert(widget->get_outermost_parent());
			widget->m_redraw_requested = false;
		}
	}

	std::vector<Asset_header*> asset_dependencies;

	for (Ui_widget* dirty_widget : ui_state.m_dirty_root_widgets)
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

						ui_state.m_dirty_root_widgets.insert(it->second->get_outermost_parent());
					}
				);

				header->m_on_loaded_delegate.try_bind(dirty_widget->m_dependencies_loaded_handle);

				should_update_render_scene = false;
				break;
				
			}
		}

		if (!should_update_render_scene)
			continue;

		auto it = ui_state.m_root_to_window_size_lut.find(dirty_widget->m_key.value());
		assert(it != ui_state.m_root_to_window_size_lut.end() && "Dirty_widget was not a properly setup root widget!");

		const glm::vec2 window_size = it->second.m_size;

		dirty_widget->calculate_content_size(window_size);
		dirty_widget->calculate_render_transform(window_size);

		dirty_widget->update_render_scene
		(
			dirty_widget->m_render_translation + dirty_widget->m_local_translation,
			dirty_widget->m_global_render_size * dirty_widget->m_local_scale,
			dirty_widget->m_render_rotation + dirty_widget->m_local_rotation,
			window_size,
			it->second.m_id,
			in_processor
		);
	}

	ui_state.m_dirty_root_widgets.clear();
}

void sic::System_ui::update_ui_interactions(Processor_ui in_processor)
{
	State_ui& ui_state = in_processor.get_state_checked_w<State_ui>();
	const State_input& input_state = in_processor.get_state_checked_r<State_input>();
	const State_window& window_state = in_processor.get_state_checked_r<State_window>();

	auto focused_window = window_state.get_focused_window();
	if (!focused_window)
		return;

	auto root_widget_it = ui_state.m_widget_lut.find(window_state.get_focused_window()->get_name());
	if (root_widget_it == ui_state.m_widget_lut.end())
		return;

	const glm::vec2& cursor_pos = focused_window->get_cursor_postition() / glm::vec2(focused_window->get_dimensions().x, focused_window->get_dimensions().y);

	std::vector<Ui_widget*> widgets_over_cursor;
	root_widget_it->second->gather_widgets_over_point(cursor_pos, widgets_over_cursor);

	auto consume_widget_it = std::find_if(widgets_over_cursor.begin(), widgets_over_cursor.end(), [](Ui_widget* in_widget) { return in_widget->m_interaction_consume_type == Interaction_consume::consume; });

	if (consume_widget_it != widgets_over_cursor.end())
		++consume_widget_it;

	auto released_button = input_state.get_released_mousebutton();

	if (focused_window->get_cursor_movement().x != 0.0f || focused_window->get_cursor_movement().y != 0.0f)
	{
		for (auto widget = widgets_over_cursor.begin(); widget != consume_widget_it; ++widget)
			(*widget)->on_cursor_move_over(cursor_pos, focused_window->get_cursor_movement());

		for (auto&& widget : ui_state.m_widgets)
		{
			Ui_widget* raw_widget = widget.get();

			if (!raw_widget || raw_widget->get_outermost_parent() != root_widget_it->second)
				continue;

			if (raw_widget->is_point_inside(cursor_pos))
			{
				if (raw_widget->m_interaction_state == Ui_interaction_state::pressed_not_hovered)
				{
					if (auto button = input_state.get_down_mousebutton())
					{
						raw_widget->on_pressed(button.value(), cursor_pos);
						raw_widget->m_interaction_state = Ui_interaction_state::pressed;
						raw_widget->on_interaction_state_changed();
					}
				}
			}

			//if its not over, or if its not above the consume widget
			if (!raw_widget->is_point_inside(cursor_pos) || std::find(widgets_over_cursor.begin(), consume_widget_it, raw_widget) == consume_widget_it)
			{
				if (raw_widget->m_interaction_state == Ui_interaction_state::hovered)
				{
					raw_widget->on_hover_end(cursor_pos);
					raw_widget->m_interaction_state = Ui_interaction_state::idle;
					raw_widget->on_interaction_state_changed();
				}
				else if (raw_widget->m_interaction_state == Ui_interaction_state::pressed)
				{
					raw_widget->on_hover_end(cursor_pos);
					raw_widget->m_interaction_state = Ui_interaction_state::pressed_not_hovered;
					raw_widget->on_interaction_state_changed();
				}
			}
		}
	}

	for (auto widget = widgets_over_cursor.begin(); widget != consume_widget_it; ++widget)
	{
		if ((*widget)->m_interaction_state == Ui_interaction_state::idle)
		{
			(*widget)->m_interaction_state = Ui_interaction_state::hovered;
			(*widget)->on_hover_begin(cursor_pos);
		}
	}

	if (auto pressed_button = input_state.get_pressed_mousebutton())
	{
		for (auto widget = widgets_over_cursor.begin(); widget != consume_widget_it; ++widget)
		{
			if ((*widget)->m_interaction_state == Ui_interaction_state::hovered)
			{
				(*widget)->m_interaction_state = Ui_interaction_state::pressed;
				(*widget)->on_pressed(pressed_button.value(), cursor_pos);
			}
		}
	}

	if (released_button)
	{
		for (auto widget = widgets_over_cursor.begin(); widget != consume_widget_it; ++widget)
		{
			if ((*widget)->m_interaction_state == Ui_interaction_state::pressed)
				(*widget)->on_clicked(released_button.value(), cursor_pos);

			(*widget)->on_released(released_button.value(), cursor_pos);

			(*widget)->m_interaction_state = Ui_interaction_state::idle;
		}
	}

	if (released_button)
	{
		for (auto&& widget : ui_state.m_widgets)
		{
			Ui_widget* raw_widget = widget.get();

			if (!raw_widget || raw_widget->get_outermost_parent() != root_widget_it->second)
				continue;

			if (raw_widget->m_interaction_state == Ui_interaction_state::pressed_not_hovered)
			{
				raw_widget->m_interaction_state = Ui_interaction_state::idle;
				raw_widget->on_interaction_state_changed();
			}
		}
	}
}

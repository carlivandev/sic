#include "sic/system_ui.h"
#include <algorithm>

sic::Log sic::g_log_ui("UI");
sic::Log sic::g_log_ui_verbose("UI", false);

void sic::Ui_widget_base::calculate_render_transform(const glm::vec2& in_window_size)
{
	in_window_size;

	if (!m_parent)
	{
		m_render_translation = { 0.0f, 0.0f };
		m_global_render_size = { 1.0f, 1.0f };
		m_render_rotation = 0.0f;
	}
}

sic::Ui_widget_base* sic::Ui_widget_base::get_outermost_parent()
{
	if (m_parent)
		return m_parent->get_outermost_parent();

	return this;
}

void sic::Ui_widget_base::get_dependencies_not_loaded(std::vector<Asset_header*>& out_assets) const
{
	gather_dependencies(out_assets);

	auto it = std::remove_if(out_assets.begin(), out_assets.end(), [](Asset_header* in_header) { return in_header->m_load_state != Asset_load_state::loaded; });
	it;
}

bool sic::Ui_widget_base::get_ready_to_be_shown() const
{
	if (m_allow_dependency_streaming)
		return true;

	std::vector<Asset_header*> not_loaded_assets;
	get_dependencies_not_loaded(not_loaded_assets);

	return not_loaded_assets.empty();
}

void sic::Ui_widget_base::destroy(State_ui& inout_ui_state)
{
	SIC_LOG(g_log_ui_verbose, "destroyed widget: {0}", m_key);
}

void sic::Ui_widget_base::destroy_render_object(State_ui& inout_ui_state, Update_list_id<Render_object_ui>& inout_ro_id) const
{
	if (!inout_ro_id.is_valid())
		return;

	SIC_LOG(g_log_ui_verbose, "destroyed RO: {0}, owner: {1}", inout_ro_id.get_id(), m_key);

	const auto it = std::find_if(inout_ui_state.m_ro_ids_to_destroy.begin(), inout_ui_state.m_ro_ids_to_destroy.end(), [inout_ro_id](auto& other) {return inout_ro_id.get_id() == other.get_id(); });
	assert(it == inout_ui_state.m_ro_ids_to_destroy.end());

	inout_ui_state.m_ro_ids_to_destroy.push_back(inout_ro_id);
	inout_ro_id.reset();
}

void sic::Ui_widget_base::update_render_object(Processor_ui& inout_processor, Update_list_id<Render_object_ui>& inout_ro_id, std::function<void(Render_object_ui&)> in_callback) const
{
	if (inout_ro_id.is_valid())
	{
		inout_processor.schedule
		(
			std::function([ro_id = inout_ro_id, callback = in_callback](Processor<Processor_flag_write<State_render_scene>> in_processor) mutable
			{
				in_processor.get_state_checked_w<State_render_scene>().m_ui_elements.update_object(ro_id, std::move(callback));
			})
		);
	}
	else
	{
		inout_processor.schedule
		(
			std::function([&ro_id = inout_ro_id, callback = in_callback](Processor<Processor_flag_write<State_render_scene>> in_processor) mutable
			{
				ro_id = in_processor.get_state_checked_w<State_render_scene>().m_ui_elements.create_object(std::move(callback));
			})
		);
	}
}

void sic::Ui_widget_base::on_cursor_move_over(const glm::vec2& in_cursor_pos, const glm::vec2& in_cursor_movement)
{
	SIC_LOG(g_log_ui_verbose, "on_cursor_move_over({0})", m_key);

	invoke<On_cursor_move_over>();

	if (m_interaction_state == Ui_interaction_state::pressed || m_interaction_state == Ui_interaction_state::pressed_not_hovered)
		invoke<On_drag>(in_cursor_pos, in_cursor_movement);
}

void sic::Ui_widget_base::on_hover_begin(const glm::vec2& in_cursor_pos)
{
	in_cursor_pos;
	SIC_LOG(g_log_ui_verbose, "on_hover_begin({0})", m_key);

	invoke<On_hover_begin>();
}

void sic::Ui_widget_base::on_hover_end(const glm::vec2& in_cursor_pos)
{
	in_cursor_pos;
	SIC_LOG(g_log_ui_verbose, "on_hover_end({0})", m_key);

	invoke<On_hover_end>();
}

void sic::Ui_widget_base::on_pressed(Mousebutton in_button, const glm::vec2& in_cursor_pos)
{
	in_button; in_cursor_pos;
	SIC_LOG(g_log_ui_verbose, "on_pressed({0})", m_key);
	
	invoke<On_pressed>();
}

void sic::Ui_widget_base::on_released(Mousebutton in_button, const glm::vec2& in_cursor_pos)
{
	in_button; in_cursor_pos;
	SIC_LOG(g_log_ui_verbose, "on_released({0})", m_key);
	
	invoke<On_released>();
}

void sic::Ui_widget_base::on_clicked(Mousebutton in_button, const glm::vec2& in_cursor_pos)
{
	in_button; in_cursor_pos;
	SIC_LOG(g_log_ui_verbose, "on_clicked({0})", m_key);
	
	invoke<On_clicked>();
}

void sic::Ui_widget_base::on_character_input(unsigned int in_character)
{
	SIC_LOG(g_log_ui_verbose, "on_character_input({0}), character: {1}", m_key, in_character);

	invoke<On_character_input>(in_character);
}

void sic::Ui_widget_base::on_focus_begin()
{
	SIC_LOG(g_log_ui_verbose, "on_focus_begin({0})", m_key);

	invoke<On_focus_begin>();
}

void sic::Ui_widget_base::on_focus_end()
{
	SIC_LOG(g_log_ui_verbose, "on_focus_end({0})", m_key);

	invoke<On_focus_end>();
}

void sic::System_ui::on_created(Engine_context in_context)
{
	in_context.register_state<State_ui>("State_ui");
}

void sic::System_ui::on_engine_tick(Engine_context in_context, float in_time_delta) const
{
	in_time_delta;
	in_context.schedule(make_functor(update_ui));
}

void sic::System_ui::update_ui(Processor_ui in_processor)
{
	State_ui& ui_state = in_processor.get_state_checked_w<State_ui>();

	//update ui before drawing, using their previous render transforms (1 frame delay if they are modified)
	update_ui_interactions(in_processor);

	for (auto&& widget : ui_state.m_widgets)
	{
		if (!widget)
			continue;

		if ((ui8)widget->m_update_requested & (ui8)Ui_widget_update::layout)
		{
			ui_state.m_dirty_root_widgets.insert(widget->get_outermost_parent());
			((ui8&)widget->m_update_requested) &= ~(ui8)Ui_widget_update::layout;
		}

		if ((ui8)widget->m_update_requested & (ui8)Ui_widget_update::appearance)
			ui_state.m_widgets_to_redraw.push_back(widget.get());
	}

	std::vector<Asset_header*> asset_dependencies;

	for (Ui_widget_base* dirty_widget : ui_state.m_dirty_root_widgets)
	{
		if (ui_state.m_widget_lut.find(dirty_widget->m_key) == ui_state.m_widget_lut.end())
			ui_state.m_widget_lut.insert_or_assign(dirty_widget->m_key, dirty_widget);

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
					[&ui_state, key = dirty_widget->m_key](Asset*)
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

		auto it = ui_state.m_root_to_window_size_lut.find(dirty_widget->m_key);
		assert(it != ui_state.m_root_to_window_size_lut.end() && "Dirty_widget was not a properly setup root widget!");

		const glm::vec2 window_size = it->second.m_size;

		dirty_widget->calculate_content_size(window_size);
		dirty_widget->calculate_render_transform(window_size);

		ui32 cur_sort_priority = 0;
		dirty_widget->calculate_sort_priority(cur_sort_priority);

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

	for (Ui_widget_base* widget_to_redraw : ui_state.m_widgets_to_redraw)
	{
		if (widget_to_redraw->m_update_requested == Ui_widget_update::none)
			continue;

		auto it = ui_state.m_root_to_window_size_lut.find(widget_to_redraw->get_outermost_parent()->m_key);
		assert(it != ui_state.m_root_to_window_size_lut.end() && "Dirty_widget was not a properly setup root widget!");

		const glm::vec2 window_size = it->second.m_size;

		widget_to_redraw->update_render_scene
		(
			widget_to_redraw->m_render_translation + widget_to_redraw->m_local_translation,
			widget_to_redraw->m_global_render_size * widget_to_redraw->m_local_scale,
			widget_to_redraw->m_render_rotation + widget_to_redraw->m_local_rotation,
			window_size,
			it->second.m_id,
			in_processor
		);
	}

	ui_state.m_widgets_to_redraw.clear();

	if (!ui_state.m_ro_ids_to_destroy.empty())
	{
		in_processor.schedule
		(
			std::function([ro_ids = ui_state.m_ro_ids_to_destroy](Processor<Processor_flag_write<State_render_scene>> in_processor) mutable
			{
				for (auto ro_id : ro_ids)
					if (ro_id.is_valid())
						in_processor.get_state_checked_w<State_render_scene>().m_ui_elements.destroy_object(ro_id);
			})
		);

		ui_state.m_ro_ids_to_destroy.clear();
	}
}

void sic::System_ui::update_ui_interactions(Processor_ui in_processor)
{
	State_ui& ui_state = in_processor.get_state_checked_w<State_ui>();
	const State_input& input_state = in_processor.get_state_checked_r<State_input>();
	const State_window& window_state = in_processor.get_state_checked_r<State_window>();

	std::vector<const Window_proxy*> windows;
	for (auto&& window_it : window_state.get_windows())
		windows.push_back(window_it.second.get());

	std::sort
	(
		windows.begin(), windows.end(),
		[](const Window_proxy* in_a, const Window_proxy* in_b)
		{
			return in_a->m_time_since_focused < in_b->m_time_since_focused;
		}
	);

	auto released_button = input_state.get_released_mousebutton();

	for (const Window_proxy* cur_window : windows)
	{
		auto root_widget_it = ui_state.m_widget_lut.find(cur_window->get_name());
		if (root_widget_it == ui_state.m_widget_lut.end())
			continue;

		const glm::vec2& cursor_pos = cur_window->get_cursor_postition() / glm::vec2(cur_window->get_dimensions().x, cur_window->get_dimensions().y);

		std::vector<Ui_widget_base*> widgets_over_cursor;
		root_widget_it->second->gather_widgets_over_point(cursor_pos, widgets_over_cursor);

		auto consume_widget_it = std::find_if(widgets_over_cursor.begin(), widgets_over_cursor.end(), [](Ui_widget_base* in_widget) { return in_widget->m_interaction_consume_type == Interaction_consume::consume; });

		if (consume_widget_it != widgets_over_cursor.end())
			++consume_widget_it;

		if (cur_window->get_cursor_movement().x != 0.0f || cur_window->get_cursor_movement().y != 0.0f)
		{
			for (auto widget = widgets_over_cursor.begin(); widget != consume_widget_it; ++widget)
				(*widget)->on_cursor_move_over(cursor_pos, cur_window->get_cursor_movement());

			for (auto&& widget : ui_state.m_widgets)
			{
				Ui_widget_base* raw_widget = widget.get();

				if (!raw_widget || raw_widget->get_outermost_parent() != root_widget_it->second || raw_widget->m_interaction_consume_type == Interaction_consume::disabled)
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
			if ((*widget)->m_interaction_state == Ui_interaction_state::idle && (*widget)->m_interaction_consume_type != Interaction_consume::disabled)
			{
				(*widget)->m_interaction_state = Ui_interaction_state::hovered;
				(*widget)->on_interaction_state_changed();
				(*widget)->on_hover_begin(cursor_pos);
			}
		}

		if (auto pressed_button = input_state.get_pressed_mousebutton())
		{
			for (auto widget = widgets_over_cursor.begin(); widget != consume_widget_it; ++widget)
			{
				if ((*widget)->m_interaction_state == Ui_interaction_state::hovered && (*widget)->m_interaction_consume_type != Interaction_consume::disabled)
				{
					if (ui_state.m_focused_widget && *widget != ui_state.m_focused_widget)
						ui_state.m_focused_widget->on_focus_end();

					(*widget)->m_interaction_state = Ui_interaction_state::pressed;
					(*widget)->on_interaction_state_changed();
					(*widget)->on_pressed(pressed_button.value(), cursor_pos);

					if (*widget != ui_state.m_focused_widget)
						(*widget)->on_focus_begin();
					ui_state.m_focused_widget = *widget;
				}
			}
		}

		if (released_button)
		{
			for (auto widget = widgets_over_cursor.begin(); widget != consume_widget_it; ++widget)
			{
				if ((*widget)->m_interaction_consume_type == Interaction_consume::disabled)
					continue;

				if ((*widget)->m_interaction_state == Ui_interaction_state::pressed)
				{
					(*widget)->on_clicked(released_button.value(), cursor_pos);
				}

				(*widget)->on_released(released_button.value(), cursor_pos);

				(*widget)->m_interaction_state = Ui_interaction_state::idle;
				(*widget)->on_interaction_state_changed();
			}
		}

		if (released_button)
		{
			for (auto&& widget : ui_state.m_widgets)
			{
				Ui_widget_base* raw_widget = widget.get();

				if (!raw_widget || raw_widget->get_outermost_parent() != root_widget_it->second)
					continue;

				if (raw_widget->m_interaction_consume_type == Interaction_consume::disabled)
					continue;

				if (raw_widget->m_interaction_state == Ui_interaction_state::pressed_not_hovered)
				{
					raw_widget->on_released(released_button.value(), cursor_pos);

					raw_widget->m_interaction_state = Ui_interaction_state::idle;
					raw_widget->on_interaction_state_changed();
				}
			}
		}

		//dont continue updating next window if we already hovered 
		if (widgets_over_cursor.size() > 0)
			break;
	}

	if (ui_state.m_focused_widget)
	{
		if (input_state.m_character_input.has_value())
			ui_state.m_focused_widget->on_character_input(input_state.m_character_input.value());

		if (input_state.is_key_down(Key::left_control) && input_state.is_key_pressed(Key::c))
		{
			auto clipboard_string = ui_state.m_focused_widget->get_clipboard_string();
			SIC_LOG(g_log_ui, "clipboard: \"{0}\"", clipboard_string.value_or("DONT SET"));
		}
	}
}

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

	((ui8&)m_update_requested) &= ~(ui8)Ui_widget_update::layout;
}

void sic::Ui_widget_base::calculate_content_size(const glm::vec2& in_window_size, bool in_slot_determines_size_x, bool in_slot_determines_size_y)
{
	if (m_visibility_state == Ui_visibility::collapsed)
	{
		if (!in_slot_determines_size_x)
			m_global_render_size.x = 0.0f;

		if (!in_slot_determines_size_y)
			m_global_render_size.y = 0.0f;
		return;
	}

	auto content_size = get_content_size(in_window_size);

	if (!in_slot_determines_size_x)
		m_global_render_size.x = content_size.x;

	if (!in_slot_determines_size_y)
		m_global_render_size.y = content_size.y;
}

sic::Ui_widget_base* sic::Ui_widget_base::get_outermost_parent()
{
	if (m_parent)
		return m_parent->get_outermost_parent();

	return this;
}

const sic::Ui_widget_base* sic::Ui_widget_base::get_outermost_parent() const
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

void sic::Ui_widget_base::destroy(Processor_ui& inout_processor)
{
	inout_processor;
	SIC_LOG(g_log_ui_verbose, "destroyed widget: {0}", m_key);
}

void sic::Ui_widget_base::destroy_render_object(Processor_ui& inout_processor, std::optional<size_t>& inout_index)
{
	if (!inout_index.has_value())
		return;

	auto& ui_state = inout_processor.get_state_checked_w<State_ui>();
	auto window_id = m_window_id;

	auto& to_remove = m_ro_ids[inout_index.value()];
	ui_state.m_free_ro_ids_this_frame.push_back(to_remove.m_ro_id.get_id());

	ui_state.m_updates.write
	(
		[&](auto& write)
		{
			write.push_back
			(
				[remove_id = to_remove.m_ro_id, window_id](Engine_processor<Processor_flag_write<State_render_scene>> in_processor) mutable
				{
					auto& lut = in_processor.get_state_checked_w<State_render_scene>().m_window_id_to_ui_elements;
					auto window_widgets = lut.find(window_id.get_id());

					if (window_widgets != lut.end())
						window_widgets->second.destroy_object(remove_id, [](Render_object_ui& inout_object) { if (inout_object.m_material) inout_object.m_material->remove_instance_data(inout_object.m_instance_data_index); });
				}
			);
		}
	);

	auto next_ro_id = to_remove.m_next_widget ? to_remove.m_next_widget->m_ro_ids[to_remove.m_next_ro_index.value()].m_ro_id : Update_list_id<Render_object_ui>();

	if (to_remove.m_next_widget)
	{
		auto& next_id = to_remove.m_next_widget->m_ro_ids[to_remove.m_next_ro_index.value()];
		next_id.m_previous_widget = to_remove.m_previous_widget;
		next_id.m_previous_ro_index = to_remove.m_previous_ro_index;
	}

	if (to_remove.m_previous_widget)
	{
		auto& prev_id = to_remove.m_previous_widget->m_ro_ids[to_remove.m_previous_ro_index.value()];
		prev_id.m_next_widget = to_remove.m_next_widget;
		prev_id.m_next_ro_index = to_remove.m_next_ro_index;

		ui_state.m_updates.write
		(
			[&](auto& write)
			{
				write.push_back
				(
					[prev_ro_id = prev_id.m_ro_id, next_ro_id, window_id](Engine_processor<Processor_flag_write<State_render_scene>> in_processor) mutable
					{
						auto& lut = in_processor.get_state_checked_w<State_render_scene>().m_window_id_to_ui_elements;
						auto window_widgets = lut.find(window_id.get_id());

						if (window_widgets != lut.end())
							window_widgets->second.update_object(prev_ro_id, [next_ro_id](Render_object_ui& ro) { ro.m_next = next_ro_id; });
					}
				);
			}
		);
	}
	else
	{
		//we were the start of the linked list, update to next
		
		ui_state.m_updates.write
		(
			[&](auto& write)
			{
				write.push_back
				(
					[new_start_id = next_ro_id, window_id](Engine_processor<Processor_flag_write<State_render_scene>> in_processor) mutable
					{
						auto& lut = in_processor.get_state_checked_w<State_render_scene>().m_window_id_to_first_ui_element;
						auto window_widgets = lut.find(window_id.get_id());

						if (window_widgets != lut.end())
							window_widgets->second = new_start_id;
					}
				);
			}
		);
	}

	m_free_ro_id_indices.push_back(inout_index.value());
	inout_index.reset();
	to_remove.m_ro_id.reset();
	to_remove.m_next_widget = nullptr;
	to_remove.m_next_ro_index.reset();
	to_remove.m_previous_widget = nullptr;
	to_remove.m_previous_ro_index.reset();
}

void sic::Ui_widget_base::update_render_object(Processor_ui& inout_processor, std::optional<size_t>& inout_index, std::function<void(Render_object_ui&)> in_callback)
{
	auto window_id = m_window_id;

	assert(window_id.is_valid());

	if (inout_index.has_value())
	{
		if (m_visibility_state != Ui_visibility::visible)
		{
			destroy_render_object(inout_processor, inout_index);
			return;
		}

		auto& ro_id = m_ro_ids[inout_index.value()].m_ro_id;

		inout_processor.get_state_checked_w<State_ui>().m_updates.write
		(
			[&](auto& write)
			{
				write.push_back
				(
					[ro_id, callback = in_callback, window_id](Engine_processor<Processor_flag_write<State_render_scene>> in_processor) mutable
					{
						auto& lut = in_processor.get_state_checked_w<State_render_scene>().m_window_id_to_ui_elements;
						auto window_widgets = lut.find(window_id.get_id());

						if (window_widgets != lut.end())
							window_widgets->second.update_object(ro_id, std::move(callback));
						//SIC_LOG(g_log_ui_verbose, "RT Updated: {0}", ro_id.get_id());
					}
				);
			}
		);

		//SIC_LOG(g_log_ui_verbose, "GT Updated: {0}, owner: {1}", inout_ro_id.get_id(), m_key);

		return;
	}

	
	if (m_visibility_state != Ui_visibility::visible)
		return;

	if (m_free_ro_id_indices.empty())
	{
		inout_index = m_ro_ids.size();
		m_ro_ids.emplace_back();
	}
	else
	{
		inout_index = m_free_ro_id_indices.back();
		m_free_ro_id_indices.pop_back();
	}

	auto& ro_id = m_ro_ids[inout_index.value()];

	auto& ui_state = inout_processor.get_state_checked_w<State_ui>();

	if (ui_state.m_free_ro_ids.empty())
	{
		ro_id.m_ro_id.set(ui_state.m_ro_id_ticker++);
	}
	else
	{
		ro_id.m_ro_id.set(ui_state.m_free_ro_ids.back());
		ui_state.m_free_ro_ids.pop_back();
	}

	ui_state.m_updates.write
	(
		[&](auto& write)
		{
			write.push_back
			(
				[new_ro_id = ro_id.m_ro_id, callback = in_callback, window_id](Engine_processor<Processor_flag_write<State_render_scene>> in_processor) mutable
				{
					auto& lut = in_processor.get_state_checked_w<State_render_scene>().m_window_id_to_ui_elements;
					auto window_widgets = lut.find(window_id.get_id());

					if (window_widgets != lut.end())
						window_widgets->second.create_object(new_ro_id, std::move(callback));
					//SIC_LOG(g_log_ui_verbose, "RT Created: {0}", inout_ro_id.get_id());
				}
			);
		}
	);

	bool found_previous_ro_id = false;

	for (i32 i = (i32)inout_index.value() - 1; i >= 0; i--)
	{
		if (m_ro_ids[i].m_ro_id.is_valid())
		{
			ro_id.m_previous_widget = this;
			ro_id.m_previous_ro_index = i;

			found_previous_ro_id = true;
			break;
		}
	}

	if (!found_previous_ro_id)
	{
		Ui_widget_base* prev_widget = get_previous_widget_render_order();

		while (!found_previous_ro_id && prev_widget)
		{
			for (i32 i = (i32)prev_widget->m_ro_ids.size() - 1; i >= 0; i--)
			{
				if (prev_widget->m_ro_ids[i].m_ro_id.is_valid())
				{
					ro_id.m_previous_widget = prev_widget;
					ro_id.m_previous_ro_index = i;

					found_previous_ro_id = true;
					break;
				}
			}

			prev_widget = prev_widget->get_previous_widget_render_order();
		}
	}

	if (found_previous_ro_id)
	{
		auto& prev = ro_id.m_previous_widget->m_ro_ids[ro_id.m_previous_ro_index.value()];

		ro_id.m_next_widget = prev.m_next_widget;
		ro_id.m_next_ro_index = prev.m_next_ro_index;

		prev.m_next_widget = this;
		prev.m_next_ro_index = inout_index.value();

		if (ro_id.m_next_widget)
		{
			auto& next = ro_id.m_next_widget->m_ro_ids[ro_id.m_next_ro_index.value()];
			next.m_previous_widget = this;
			next.m_previous_ro_index = inout_index.value();
		}
	}
	else
	{
		bool cur_widget_had_next = false;

		for (i32 i = inout_index.value() + 1; i < m_ro_ids.size(); ++i)
		{
			if (m_ro_ids[i].m_ro_id.is_valid())
			{
				ro_id.m_next_widget = this;
				ro_id.m_next_ro_index = i;

				m_ro_ids[i].m_previous_ro_index = inout_index;
				m_ro_ids[i].m_previous_widget = this;
				cur_widget_had_next = true;
				break;
			}
		}

		if (!cur_widget_had_next)
		{
			Ui_widget_base* next_widget = get_next_widget_render_order();
			while (next_widget)
			{
				bool next_widget_has_valid_ro = false;

				for (size_t i = 0; i < next_widget->m_ro_ids.size(); ++i)
				{
					if (next_widget->m_ro_ids[i].m_ro_id.is_valid())
					{
						ro_id.m_next_widget = next_widget;
						ro_id.m_next_ro_index = i;

						next_widget->m_ro_ids[i].m_previous_ro_index = inout_index;
						next_widget->m_ro_ids[i].m_previous_widget = this;

						next_widget_has_valid_ro = true;
						break;
					}
				}

				if (next_widget_has_valid_ro)
					break;

				next_widget = next_widget->get_next_widget_render_order();
			}
		}

		//there is no previous ui_element, we are the start of the linked list
		ui_state.m_updates.write
		(
			[&](auto& write)
			{
				write.push_back
				(
					[new_ro_id = ro_id.m_ro_id, window_id](Engine_processor<Processor_flag_write<State_render_scene>> in_processor) mutable
					{
						auto& lut = in_processor.get_state_checked_w<State_render_scene>().m_window_id_to_first_ui_element;
						auto window_widgets = lut.find(window_id.get_id());

						if (window_widgets != lut.end())
							window_widgets->second = new_ro_id;
					}
				);
			}
		);
	}

	auto next_ro_id = ro_id.m_next_widget ? ro_id.m_next_widget->m_ro_ids[ro_id.m_next_ro_index.value()].m_ro_id : Update_list_id<Render_object_ui>();
	auto previous_ro_id = ro_id.m_previous_widget ? ro_id.m_previous_widget->m_ro_ids[ro_id.m_previous_ro_index.value()].m_ro_id : Update_list_id<Render_object_ui>();

	ui_state.m_updates.write
	(
		[&](auto& write)
		{
			write.push_back
			(
				[new_ro_id = ro_id.m_ro_id, previous_ro_id, next_ro_id, window_id](Engine_processor<Processor_flag_write<State_render_scene>> in_processor) mutable
				{
					auto& lut = in_processor.get_state_checked_w<State_render_scene>().m_window_id_to_ui_elements;
					auto window_widgets = lut.find(window_id.get_id());

					if (window_widgets != lut.end())
					{
						if (previous_ro_id.is_valid())
							window_widgets->second.update_object(previous_ro_id, [new_ro_id](Render_object_ui& ro) { ro.m_next = new_ro_id; });

						if (next_ro_id.is_valid())
							window_widgets->second.update_object(new_ro_id, [next_ro_id](Render_object_ui& ro) { ro.m_next = next_ro_id; });
					}
				}
			);
		}
	);
	//SIC_LOG(g_log_ui_verbose, "GT Created: {0}, owner: {1}", inout_ro_id.get_id(), m_key);
}

void sic::Ui_widget_base::set_window_id(const Update_list_id<Render_object_window>& in_window_id)
{
	m_window_id = in_window_id;
}

void sic::Ui_widget_base::Update_image_common(Processor_ui& inout_processor, Ui_widget_base& inout_owner, std::optional<size_t>& inout_ro_id, const Asset_ref<Asset_material>& in_material, const glm::vec4& in_tint, const glm::vec2& in_final_translation, const glm::vec2& in_final_size, float in_final_rotation, const glm::vec2& in_window_size, Update_list_id<Render_object_window> in_window_id)
{
	if (!in_material.is_valid())
		return;

	if (in_final_size.x == 0.0f || in_final_size.y == 0.0f)
	{
		inout_owner.destroy_render_object(inout_processor, inout_ro_id);
		return;
	}

	auto update_lambda =
		[in_final_translation, in_final_size, in_final_rotation, mat = in_material, in_window_size, tint = in_tint](Render_object_ui& inout_object)
	{
		if (inout_object.m_material != mat.get_mutable())
		{
			if (inout_object.m_material)
				inout_object.m_material->remove_instance_data(inout_object.m_instance_data_index);

			inout_object.m_instance_data_index = mat.get_mutable()->add_instance_data();
			inout_object.m_material = mat.get_mutable();
		}

		struct Local
		{
			static glm::vec2 round_to_pixel_density(const glm::vec2& in_vec, const glm::vec2& in_pixel_density)
			{
				glm::vec2 ret_val = { in_vec.x * in_pixel_density.x, in_vec.y * in_pixel_density.y };
				ret_val = glm::round(ret_val);
				ret_val.x /= in_pixel_density.x;
				ret_val.y /= in_pixel_density.y;

				return ret_val;
			}
		};

		const glm::vec2 top_left = Local::round_to_pixel_density(in_final_translation, in_window_size);
		const glm::vec2 bottom_right = Local::round_to_pixel_density(in_final_translation + in_final_size, in_window_size);

		const glm::vec4 lefttop_rightbottom_packed = { top_left.x, top_left.y, bottom_right.x, bottom_right.y };

		mat.get_mutable()->set_parameter_on_instance("lefttop_rightbottom_packed", lefttop_rightbottom_packed, inout_object.m_instance_data_index);
		mat.get_mutable()->set_parameter_on_instance("tint", tint, inout_object.m_instance_data_index);
	};

	inout_owner.update_render_object(inout_processor, inout_ro_id, update_lambda);
}

void sic::Ui_widget_base::on_cursor_move_over(const glm::vec2& in_cursor_pos, const glm::vec2& in_cursor_movement)
{
	SIC_LOG(g_log_ui_verbose, "on_cursor_move_over({0})", m_key);

	invoke<On_cursor_move_over>();

	if (m_interaction_state == Ui_interaction_state::pressed || m_interaction_state == Ui_interaction_state::pressed_not_hovered)
	{
		SIC_LOG(g_log_ui_verbose, "on_cursor_drag({0}): x:{1}, y:{2}", m_key, in_cursor_movement.x, in_cursor_movement.y);
		invoke<On_drag>(in_cursor_pos, in_cursor_movement);
	}
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

void sic::State_ui::set_window_info(const std::string& in_window_widget_key, const glm::vec2& in_size)
{
	auto root_key_it = m_window_key_to_root_key_lut.find(in_window_widget_key);
	assert(root_key_it != m_window_key_to_root_key_lut.end());

	auto it = m_root_to_window_info_lut.find(root_key_it->second);
	assert(it != m_root_to_window_info_lut.end());

	it->second.m_size = in_size;

	Ui_widget_canvas* canvas = find<Ui_widget_canvas>(in_window_widget_key);
	assert(canvas);
	canvas->m_reference_dimensions = in_size;

	Ui_widget_canvas* tooltip_canvas = find<Ui_widget_canvas>(it->second.m_tooltip_canvas_key);
	assert(tooltip_canvas);
	tooltip_canvas->m_reference_dimensions = in_size;
}

void sic::State_ui::create_window_info(const std::string& in_window_widget_key, const glm::vec2& in_size, Update_list_id<Render_object_window> in_id)
{
	Ui_widget_canvas& canvas = create<Ui_widget_canvas>(in_window_widget_key);
	canvas.m_reference_dimensions = in_size;
	canvas.interaction_consume(Interaction_consume::fall_through);

	Ui_widget_canvas& tooltip_canvas = create<Ui_widget_canvas>({});
	tooltip_canvas.m_reference_dimensions = in_size;
	tooltip_canvas.interaction_consume(Interaction_consume::fall_through);

	std::string root_key;
	create_get_key<Ui_widget_canvas>(root_key)
		.reference_dimensions({ 1.0f, 1.0f })
		.interaction_consume(Interaction_consume::fall_through)

		.add_child(Ui_widget_canvas::Slot().size({ 1.0f, 1.0f }).anchors(Ui_anchors(0.0f)), canvas)
		.add_child(Ui_widget_canvas::Slot().size({ 1.0f, 1.0f }).anchors(Ui_anchors(0.0f)), tooltip_canvas)

		.set_window_id(in_id)
	;

	m_window_key_to_root_key_lut[in_window_widget_key] = root_key;
	{
		auto&& it = m_root_to_window_info_lut[root_key];
		it.m_size = in_size;
		it.m_id = in_id;
		it.m_tooltip_canvas_key = tooltip_canvas.get_key();
	}
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

	for (auto&& widget : ui_state.m_widgets)
	{
		if (!widget)
			continue;

		Ui_widget_update update = widget->m_update_requested;

		if (widget->m_parent)
		{
			auto& slot_update = *widget->m_parent->find_slot_update_requested(widget->m_slot_index);
			((ui8&)update) |= (ui8)slot_update;
			slot_update = Ui_widget_update::none;
		}

		if ((ui8)update & (ui8)Ui_widget_update::layout)
		{
			Ui_widget_base* root_widget = widget.get();

			while (root_widget->get_parent())
			{
				if (!root_widget->should_propagate_dirtyness_to_parent())
					break;

				root_widget = root_widget->get_parent();
			}

			//Ui_widget_base* root_widget = widget->get_outermost_parent();

			ui_state.m_dirty_root_widgets.insert(root_widget);
		}

		if ((ui8)update & (ui8)Ui_widget_update::appearance)
			ui_state.m_widgets_to_redraw.push_back(widget.get());

		widget->m_render_transform_was_updated_this_frame = false;
		widget->m_render_state_was_updated_this_frame = false;

		if (update != Ui_widget_update::none)
			widget->propagate_request_to_children(update);

		widget->m_update_requested = update;
	}

	std::vector<Asset_header*> asset_dependencies;

	std::vector<Ui_widget_base*> dirty_widgets(ui_state.m_dirty_root_widgets.size());
	std::copy(ui_state.m_dirty_root_widgets.begin(), ui_state.m_dirty_root_widgets.end(), dirty_widgets.begin());

	bool should_update_render_scene = true;

	for (Ui_widget_base* dirty_widget : dirty_widgets)
	{
		if (ui_state.m_widget_lut.find(dirty_widget->m_key) == ui_state.m_widget_lut.end())
			ui_state.m_widget_lut.insert_or_assign(dirty_widget->m_key, dirty_widget);

		asset_dependencies.clear();
		dirty_widget->gather_dependencies(asset_dependencies);

		for (Asset_header* header : asset_dependencies)
		{
			if (header->m_load_state != Asset_load_state::loaded)
			{
				//postpone render object update until loaded

				dirty_widget->m_dependencies_loaded_handle.set_callback
				(
					[&ui_state, key = dirty_widget->m_key](Asset_loaded_processor, Asset*)
					{
						auto it = ui_state.m_widget_lut.find(key);
						if (it == ui_state.m_widget_lut.end())
							return;

						if (auto* widget = it->second)
							widget->request_update(Ui_widget_update::layout_and_appearance);
					}
				);

				header->m_on_loaded_delegate.try_bind(dirty_widget->m_dependencies_loaded_handle);

				should_update_render_scene = false;
				break;
				
			}
		}

		if (!should_update_render_scene)
			break;

		auto it = ui_state.m_root_to_window_info_lut.find(dirty_widget->get_outermost_parent()->m_key);
		assert(it != ui_state.m_root_to_window_info_lut.end() && "Dirty_widget was not a properly setup root widget!");

		const glm::vec2 window_size = it->second.m_size;

		if (!dirty_widget->m_render_transform_was_updated_this_frame)
		{
			if (auto* parent = dirty_widget->get_parent())
			{
				auto slot_overrides = parent->get_slot_overrides_content_size(dirty_widget->get_slot_index());
				dirty_widget->calculate_content_size(window_size, slot_overrides.first, slot_overrides.second);
			}
			else
			{
				dirty_widget->calculate_content_size(window_size, true, true);
			}

			dirty_widget->calculate_render_transform(window_size);

			dirty_widget->m_render_transform_was_updated_this_frame = true;
		}

		if (!dirty_widget->m_render_state_was_updated_this_frame)
		{
			dirty_widget->update_render_scene
			(
				dirty_widget->m_render_translation + dirty_widget->m_local_translation,
				dirty_widget->m_global_render_size * dirty_widget->m_local_scale,
				dirty_widget->m_render_rotation + dirty_widget->m_local_rotation,
				window_size,
				it->second.m_id,
				in_processor
			);

			dirty_widget->m_render_state_was_updated_this_frame = true;
		}
	}

	ui_state.m_dirty_root_widgets.clear();

	if (should_update_render_scene)
	{
		for (Ui_widget_base* widget_to_redraw : ui_state.m_widgets_to_redraw)
		{
			if (widget_to_redraw->m_update_requested == Ui_widget_update::none)
				continue;

			auto it = ui_state.m_root_to_window_info_lut.find(widget_to_redraw->get_outermost_parent()->m_key);
			assert(it != ui_state.m_root_to_window_info_lut.end() && "Dirty_widget was not a properly setup root widget!");

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
	}

	ui_state.m_widgets_to_redraw.clear();

	std::vector<std::function<void(Engine_processor<Processor_flag_write<State_render_scene>>)>> updates;

	ui_state.m_updates.swap([&](auto& read, auto& write) { read.clear(); updates = write; });

	in_processor.schedule
	(
		[&updates = ui_state.m_updates](Engine_processor<Processor_flag_write<State_render_scene>> in_processor) mutable
		{
			updates.read([&](auto& read)
			{
				for (auto&& update : read)
					update(in_processor);
			});
		}
	);

	std::copy(ui_state.m_free_ro_ids_this_frame.begin(), ui_state.m_free_ro_ids_this_frame.end(), std::back_inserter(ui_state.m_free_ro_ids));
	ui_state.m_free_ro_ids_this_frame.clear();
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
		auto root_widget_it = ui_state.m_widget_lut.find(ui_state.m_window_key_to_root_key_lut.find(cur_window->get_name())->second);
		if (root_widget_it == ui_state.m_widget_lut.end())
			continue;

		const glm::vec2& cursor_pos = cur_window->get_cursor_postition() / glm::vec2(cur_window->get_dimensions().x, cur_window->get_dimensions().y);

		std::vector<Ui_widget_base*> widgets_over_cursor;
		root_widget_it->second->gather_widgets_over_point(cursor_pos, widgets_over_cursor);

		auto consume_widget_it = std::find_if(widgets_over_cursor.begin(), widgets_over_cursor.end(), [](Ui_widget_base* in_widget) { return in_widget->m_interaction_consume_type == Interaction_consume::consume; });

		if (consume_widget_it != widgets_over_cursor.end())
			++consume_widget_it;

		if (cur_window->get_cursor_movement().x != 0.0f || cur_window->get_cursor_movement().y != 0.0f)
			for (auto widget = widgets_over_cursor.begin(); widget != consume_widget_it; ++widget)
				(*widget)->on_cursor_move_over(cursor_pos, cur_window->get_cursor_movement());

		for (auto&& widget : ui_state.m_widgets)
		{
			Ui_widget_base* raw_widget = widget.get();

			if (!raw_widget || raw_widget->get_outermost_parent() != root_widget_it->second)
				continue;

			if (raw_widget->m_interaction_consume_type != Interaction_consume::disabled && raw_widget->is_point_inside(cursor_pos))
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

			bool over = raw_widget->is_point_inside(cursor_pos);
			bool above_consume = std::find(widgets_over_cursor.begin(), consume_widget_it, raw_widget) != consume_widget_it;

			//if its not over, or if its not above the consume widget
			if (!over || !above_consume || raw_widget->m_interaction_consume_type == Interaction_consume::disabled)
			{
				if (raw_widget->m_interaction_state == Ui_interaction_state::hovered)
				{
					raw_widget->on_hover_end(cursor_pos);
					raw_widget->m_interaction_state = Ui_interaction_state::idle;
					raw_widget->on_interaction_state_changed();

					if (auto cb = raw_widget->m_on_create_tooltip_callback)
					{
						const std::string& tooltip_key = ui_state.m_root_to_window_info_lut.find(ui_state.m_window_key_to_root_key_lut.find(cur_window->get_name())->second)->second.m_tooltip_canvas_key;
						Ui_widget_canvas* canvas = ui_state.find<Ui_widget_canvas>(tooltip_key);
						canvas->destroy_children(in_processor);
					}
				}
				else if (raw_widget->m_interaction_state == Ui_interaction_state::pressed)
				{
					raw_widget->on_hover_end(cursor_pos);
					raw_widget->m_interaction_state = Ui_interaction_state::pressed_not_hovered;
					raw_widget->on_interaction_state_changed();

					if (auto cb = raw_widget->m_on_create_tooltip_callback)
					{
						const std::string& tooltip_key = ui_state.m_root_to_window_info_lut.find(ui_state.m_window_key_to_root_key_lut.find(cur_window->get_name())->second)->second.m_tooltip_canvas_key;
						Ui_widget_canvas* canvas = ui_state.find<Ui_widget_canvas>(tooltip_key);
						canvas->destroy_children(in_processor);
					}
				}
			}
		}

		for (auto widget = widgets_over_cursor.begin(); widget != consume_widget_it; ++widget)
		{
			Ui_widget_base* raw_widget = (*widget);

			if (raw_widget->m_interaction_state == Ui_interaction_state::idle && raw_widget->m_interaction_consume_type != Interaction_consume::disabled && raw_widget->m_visibility_state == Ui_visibility::visible)
			{
				raw_widget->m_interaction_state = Ui_interaction_state::hovered;
				raw_widget->on_interaction_state_changed();
				raw_widget->on_hover_begin(cursor_pos);

				if (auto cb = raw_widget->m_on_create_tooltip_callback)
				{
					const std::string& tooltip_key = ui_state.m_root_to_window_info_lut.find(ui_state.m_window_key_to_root_key_lut.find(cur_window->get_name())->second)->second.m_tooltip_canvas_key;
					Ui_widget_canvas* canvas = ui_state.find<Ui_widget_canvas>(tooltip_key);

					auto pair = cb(ui_state, canvas->m_reference_dimensions);

					canvas->destroy_children(in_processor);
					canvas->add_child
					(
						Ui_widget_canvas::Slot().anchors(Ui_anchors(0.0f)).translation(raw_widget->m_render_translation * canvas->m_reference_dimensions).size(raw_widget->m_global_render_size * canvas->m_reference_dimensions),
						ui_state.create<Ui_widget_tooltip>({})
						.cursor_translation(cur_window->get_cursor_postition())
						.interaction_consume(Interaction_consume::disabled)
						.add_child
						(
							pair.second,
							pair.first
						)
					);
				}
			}
		}

		if (auto pressed_button = input_state.get_pressed_mousebutton())
		{
			for (auto widget = widgets_over_cursor.begin(); widget != consume_widget_it; ++widget)
			{
				if ((*widget)->m_interaction_state == Ui_interaction_state::hovered && (*widget)->m_interaction_consume_type != Interaction_consume::disabled && (*widget)->m_visibility_state == Ui_visibility::visible)
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

				if ((*widget)->m_visibility_state != Ui_visibility::visible)
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

				if (raw_widget->m_visibility_state != Ui_visibility::visible)
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

	if (!ui_state.m_events_to_invoke.empty())
	{
		in_processor.schedule
		(
			[events_to_invoke = ui_state.m_events_to_invoke](Engine_processor<> in_processor)
			{
				for (auto&& event : events_to_invoke)
					event(in_processor);
			}
		);

		ui_state.m_events_to_invoke.clear();
	}
}

void sic::align_horizontal(Ui_h_alignment in_alignment, float in_max_size, float& inout_current_x, float& inout_current_size)
{
	switch (in_alignment)
	{
	case Ui_h_alignment::fill:
		inout_current_size = in_max_size;
		break;
	case Ui_h_alignment::left:
		inout_current_size = glm::min(inout_current_size, in_max_size);
		break;
	case Ui_h_alignment::right:
		inout_current_size = glm::min(inout_current_size, in_max_size);
		inout_current_x += in_max_size - inout_current_size;
		break;
	case Ui_h_alignment::center:
		inout_current_size = glm::min(inout_current_size, in_max_size);
		inout_current_x += (in_max_size - inout_current_size) * 0.5f;
		break;
	default:
		break;
	}
}

void sic::align_vertical(Ui_v_alignment in_alignment, float in_max_size, float& inout_current_y, float& inout_current_size)
{
	switch (in_alignment)
	{
	case Ui_v_alignment::fill:
		inout_current_size = in_max_size;
		break;
	case Ui_v_alignment::top:
		inout_current_size = glm::min(inout_current_size, in_max_size);
		break;
	case Ui_v_alignment::bottom:
		inout_current_size = glm::min(inout_current_size, in_max_size);
		inout_current_y += in_max_size - inout_current_size;
		break;
	case Ui_v_alignment::center:
		inout_current_size = glm::min(inout_current_size, in_max_size);
		inout_current_y += (in_max_size - inout_current_size) * 0.5f;
		break;
	default:
		break;
	}
}

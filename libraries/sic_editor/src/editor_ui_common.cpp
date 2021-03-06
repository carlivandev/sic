#include "sic/editor/editor_ui_common.h"
#include "sic/core/random.h"
#include "sic/color.h"

sic::Ui_widget_base& sic::editor::common::create_toggle_box(State_ui& inout_state, const State_editor_ui_resources& in_resources, const std::string& in_name)
{
	const std::string child_name = in_name + "_test_toggle_image";

	auto toggle_callback = [&inout_state, &in_resources, child_name](Engine_processor<>, bool in_is_toggled)
	{
		Ui_widget_image* child = inout_state.find<Ui_widget_image>(child_name);
		child->tint(in_is_toggled ? in_resources.m_foreground_color_medium : glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
	};

	inout_state.bind<Ui_widget_button::On_toggled>(in_name, toggle_callback);

	auto& ret_val = inout_state.create<Ui_widget_button>(in_name)
		.size({ 18.0f, 18.0f })
		.base_material(in_resources.m_default_material)
		.tints(in_resources.m_foreground_color_darkest, in_resources.m_foreground_color_darker, in_resources.m_foreground_color_dark)
		.is_toggle(true)
		.add_child
		(
			Ui_slot_button()
			.padding(Ui_padding(3.0f)),

			inout_state.create<Ui_widget_image>(child_name)
			.image_size({ 16.0f, 16.0f })
			.material(in_resources.m_default_material)
			.tint(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f))
			.interaction_consume(Interaction_consume::fall_through)
		)
	;

	return ret_val;
}

sic::Ui_widget_base& sic::editor::common::create_toggle_slider(State_ui& inout_state, const State_editor_ui_resources& in_resources, const std::string& in_name)
{
	const std::string child_name = in_name + "_test_toggle_image";

	auto toggle_callback = [&inout_state, &in_resources, child_name, in_name](Engine_processor<>, bool in_is_toggled)
	{
		Ui_widget_button* toggle = inout_state.find<Ui_widget_button>(in_name);
		toggle->get_child(0).first.h_align(in_is_toggled ? Ui_h_alignment::right : Ui_h_alignment::left);
		toggle->get_child(0).second.get().request_update(Ui_widget_update::appearance);
		toggle->tint_idle(in_is_toggled ? in_resources.m_accent_color_medium : in_resources.m_foreground_color_darkest);
		toggle->tint_hovered(in_is_toggled ? in_resources.m_accent_color_light : in_resources.m_foreground_color_darker);
	};

	inout_state.bind<Ui_widget_button::On_toggled>(in_name, toggle_callback);

	auto& ret_val = inout_state.create<Ui_widget_button>(in_name)
		.size({ 28.0f, 14.0f })
		.base_material(in_resources.m_default_material)
		.tints(in_resources.m_foreground_color_darkest, in_resources.m_foreground_color_darker, in_resources.m_foreground_color_dark)
		.is_toggle(true)
		.add_child
		(
			Ui_slot_button()
			.padding(Ui_padding(2.0f))
			.h_align(Ui_h_alignment::left),

			inout_state.create<Ui_widget_image>(child_name)
			.image_size({ 12.0f, 12.0f })
			.material(in_resources.m_default_material)
			.tint(in_resources.m_foreground_color_medium)
			.interaction_consume(Interaction_consume::fall_through)
		)
	;

	return ret_val;
}

sic::Ui_widget_base& sic::editor::common::create_input_box(State_ui& inout_state, const State_editor_ui_resources& in_resources, const std::string& in_name)
{
	inout_state.bind<Ui_widget_base::On_character_input>
	(
		in_name,
		[&inout_state, in_name](Engine_processor<>, unsigned int in)
		{
			Ui_widget_text* text = inout_state.find<Ui_widget_text>(in_name);
			text->text(std::string(text->get_text()) += static_cast<char>(in));
			text->request_update(Ui_widget_update::layout_and_appearance);
		}
	);

	auto& ret_val = inout_state.create<Ui_widget_text>(in_name)
		.text("enter text here\nokawf\nverycooool")
		.font(in_resources.m_default_font)
		.material(in_resources.m_default_text_material)
		.px(32.0f)
		.align(Ui_h_alignment::left)
		.selectable(true)
		.selection_box_material(in_resources.m_default_material)
		.selection_box_tint(in_resources.m_accent_color_medium)
	;

	return ret_val;
}

sic::Ui_widget_base& sic::editor::common::create_graph(const Graph_info& in_info, State_ui& inout_state)
{
	auto& parent_timelines = inout_state.create<Ui_widget_vertical_box>(in_info.m_name + "_timelines");

	auto& ret_val = inout_state.create<Ui_widget_horizontal_box>({})
		.interaction_consume(Interaction_consume::disabled)
		.add_child(Ui_slot_horizontal_box().v_align(Ui_v_alignment::top).size_fill(1.0f), parent_timelines)
	;

	const float total_time = in_info.m_end_time - in_info.m_start_time;

	for (auto&& row : in_info.m_rows)
	{
		const float graph_width = 100.0f;
		const float row_height = 32.0f;

		auto& item_canvas = inout_state.create<Ui_widget_canvas>({}).reference_dimensions({ graph_width, row_height }).interaction_consume(Interaction_consume::disabled);

		item_canvas.add_child
		(
			Ui_slot_canvas().pivot({ 0.0f, 0.0f }).anchors(Ui_anchors(0.0f, 0.0f)).size({ graph_width, row_height }),
			inout_state.create<Ui_widget_image>({}).material(in_info.m_box_material).tint(sic::Color(180, 0, 0, 255).to_vec4()).interaction_consume(Interaction_consume::disabled)
		);

		for (auto&& row_item : row.m_items)
		{
			const sic::Color green_color(0, 180, 0, 255);

			const float x_start = (row_item.m_start_time - in_info.m_start_time) / total_time;
			const float x_end = (row_item.m_end_time - in_info.m_start_time) / total_time;
			
			auto slot_data = Ui_slot_canvas().pivot({ 0.0f, 0.0f }).anchors(Ui_anchors(0.0f, 0.0f)).translation({ x_start * graph_width, 0.0f }).size({ (x_end - x_start) * graph_width, row_height });

			item_canvas.add_child
			(
				slot_data,
				inout_state.create<Ui_widget_image>({}).material(in_info.m_box_material).tint(green_color.to_vec4()).interaction_consume(Interaction_consume::consume)
				.tooltip
				(
					std::function([box_mat = in_info.m_box_material, name = row_item.m_name, font = in_info.m_font, text_mat = in_info.m_text_material](State_ui& inout_state, Ui_widget_panel& out_panel, Ui_slot_tooltip& out_slot, const glm::vec2&)
					{
						out_panel.interaction_consume(Interaction_consume::disabled);
						out_slot.pivot({ 1.0f, 1.0f }).attachment_pivot_y(0.0f);
						out_panel.add_child
						(
							Ui_widget_panel::Slot(),
							inout_state.create<Ui_widget_image>({}).material(box_mat).tint(sic::Color::black().a(128).to_vec4()).interaction_consume(Interaction_consume::disabled)
						);

						out_panel.add_child
						(
							Ui_widget_panel::Slot(),
							inout_state.create<Ui_widget_text>({}).text(name).px(16.0f).font(font).material(text_mat).foreground_color(sic::Color::white().to_vec4()).interaction_consume(Interaction_consume::disabled)
						);
					})
				)
			);

			item_canvas.add_child
			(
				slot_data,
				inout_state.create<Ui_widget_text>({}).text(" " + row_item.m_name).px(16.0f).font(in_info.m_font).material(in_info.m_text_material).foreground_color(sic::Color::white().to_vec4()).interaction_consume(Interaction_consume::disabled)
			);
		}

		parent_timelines.add_child
		(
			Ui_slot_vertical_box().h_align(Ui_h_alignment::left).padding(Ui_padding(0.0f, 0.0f, 0.0f, 1.0f)),
			inout_state.create<Ui_widget_text>({}).text(row.m_name).px(16.0f).font(in_info.m_font).material(in_info.m_text_material).foreground_color(sic::Color::white().to_vec4()).interaction_consume(Interaction_consume::disabled)
		);
		parent_timelines.add_child
		(
			Ui_slot_vertical_box().h_align(Ui_h_alignment::fill).padding(Ui_padding(0.0f, 0.0f, 0.0f, 2.0f)),
			item_canvas
		);
	}

	parent_timelines.add_child
	(
		Ui_slot_vertical_box().h_align(Ui_h_alignment::left).padding(Ui_padding(0.0f, 2.0f, 0.0f, 0.0f)),
		inout_state.create<Ui_widget_text>({}).text("Total time: " + std::to_string(total_time)).px(16.0f).font(in_info.m_font).material(in_info.m_text_material).foreground_color(sic::Color::white().to_vec4())
	);
	
	return ret_val;
}

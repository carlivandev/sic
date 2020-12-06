#include "sic/editor/editor_ui_common.h"

sic::Ui_widget& sic::editor::common::create_toggle_box(State_ui& inout_state, const State_editor_ui_resources& in_resources, const std::string& in_name)
{
	const std::string child_name = in_name + "_test_toggle_image";

	auto toggle_callback = [&inout_state, &in_resources, child_name](bool in_is_toggled)
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

sic::Ui_widget& sic::editor::common::create_toggle_slider(State_ui& inout_state, const State_editor_ui_resources& in_resources, const std::string& in_name)
{
	const std::string child_name = in_name + "_test_toggle_image";

	auto toggle_callback = [&inout_state, &in_resources, child_name, in_name](bool in_is_toggled)
	{
		Ui_widget_button* toggle = inout_state.find<Ui_widget_button>(in_name);
		toggle->get_child(0).first.h_align(in_is_toggled ? Ui_h_alignment::right : Ui_h_alignment::left);
		toggle->get_child(0).second.request_update(Ui_widget_update::appearance);
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

sic::Ui_widget& sic::editor::common::create_input_box(State_ui& inout_state, const State_editor_ui_resources& in_resources, const std::string& in_name)
{
	inout_state.bind<Ui_widget::On_character_input>
	(
		in_name,
		[&inout_state, in_name](unsigned int in)
		{
			Ui_widget_text* text = inout_state.find<Ui_widget_text>(in_name);
			text->m_text += static_cast<char>(in);
			text->request_update(Ui_widget_update::layout_and_appearance);
		}
	);

	auto& ret_val = inout_state.create<Ui_widget_text>(in_name)
		.text("enter text here")
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

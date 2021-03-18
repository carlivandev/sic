#pragma once
#include "sic/core/system.h"

#include "sic/system_window.h"
#include "sic/asset_material.h"
#include "sic/system_ui.h"

namespace sic::editor::common
{
	struct State_editor_ui_resources : State
	{
		Asset_ref<Asset_material> m_minimize_icon_material;
		Asset_ref<Asset_material> m_maximize_icon_material;
		Asset_ref<Asset_material> m_close_icon_material;

		Asset_ref<Asset_material> m_default_material;
		Asset_ref<Asset_material> m_sic_icon_material;

		Asset_ref<Asset_font> m_default_font;
		Asset_ref<Asset_material> m_default_text_material;

		glm::vec4 m_background_color_light = glm::vec4(47.0f, 47.0f, 47.0f, 255.0f) / 255.0f;
		glm::vec4 m_background_color_medium = glm::vec4(40.0f, 40.0f, 40.0f, 255.0f) / 255.0f;
		glm::vec4 m_background_color_dark = glm::vec4(33.0f, 33.0f, 33.0f, 255.0f) / 255.0f;
		glm::vec4 m_background_color_darkest = glm::vec4(23.0f, 23.0f, 23.0f, 255.0f) / 255.0f;

		glm::vec4 m_foreground_color_light = glm::vec4(255.0f, 255.0f, 255.0f, 255.0f) / 255.0f;
		glm::vec4 m_foreground_color_medium = glm::vec4(188.0f, 188.0f, 188.0f, 255.0f) / 255.0f;
		glm::vec4 m_foreground_color_dark = glm::vec4(164.0f, 164.0f, 164.0f, 255.0f) / 255.0f;
		glm::vec4 m_foreground_color_darker = glm::vec4(84.0f, 84.0f, 84.0f, 255.0f) / 255.0f;
		glm::vec4 m_foreground_color_darkest = glm::vec4(71.0f, 71.0f, 71.0f, 255.0f) / 255.0f;

		glm::vec4 m_accent_color_light = glm::vec4(0.0f, 140.0f, 178.0f, 255.0f) / 255.0f;
		glm::vec4 m_accent_color_medium = glm::vec4(0.0f, 121.0f, 154.0f, 255.0f) / 255.0f;
		glm::vec4 m_accent_color_dark = glm::vec4(0.0f, 121.0f, 154.0f, 255.0f) / 255.0f;

		glm::vec4 m_error_color_light = glm::vec4(255.0f, 122.0f, 122.0f, 255.0f) / 255.0f;
		glm::vec4 m_error_color_medium = glm::vec4(255.0f, 0.0f, 0.0f, 255.0f) / 255.0f;
		glm::vec4 m_error_color_dark = glm::vec4(128.0f, 0.0f, 0.0f, 255.0f) / 255.0f;
	};

	Ui_widget_base& create_toggle_box(State_ui& inout_state, const State_editor_ui_resources& in_resources, const std::string& in_name);
	Ui_widget_base& create_toggle_slider(State_ui& inout_state, const State_editor_ui_resources& in_resources, const std::string& in_name);
	Ui_widget_base& create_input_box(State_ui& inout_state, const State_editor_ui_resources& in_resources, const std::string& in_name);
}
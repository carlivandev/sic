#include "sic/editor/system_editor.h"

#include "sic/editor/asset_import.h"
#include "sic/editor/editor_ui_common.h"

#include "sic/core/engine_context.h"

#include "sic/system_asset.h"
#include "sic/asset_texture.h"
#include "sic/system_ui.h"

void sic::editor::System_editor::on_created(Engine_context in_context)
{
	in_context.register_state<State_editor>("State_editor");
	in_context.register_state<common::State_editor_ui_resources>("State_editor_ui_resources");
}

void sic::editor::System_editor::on_engine_finalized(Engine_context in_context) const
{
	common::State_editor_ui_resources& editor_resources = in_context.get_state_checked<common::State_editor_ui_resources>();
	State_assetsystem* assetsystem_state = in_context.get_state<State_assetsystem>();

	auto create_test_texture = [&](const std::string& texture_name, const std::string& texture_source, bool in_is_ui_image = false) -> Asset_ref<Asset_texture>
	{
		auto ref = assetsystem_state->create_asset<Asset_texture>(texture_name);
		assetsystem_state->modify_asset<Asset_texture>
		(
			ref,
			[&texture_source, in_is_ui_image](Asset_texture* texture)
			{
				sic::import_texture(*texture, File_management::load_file(texture_source), true, in_is_ui_image);
					
				if (in_is_ui_image)
				{
					texture->m_mag_filter = Texture_mag_filter::nearest;
					texture->m_min_filter = Texture_min_filter::nearest;
				}
			}
		);

		assetsystem_state->save_asset<Asset_texture>(ref, "content/textures/crysis");

		return ref;
	};

	auto create_test_ui_material = [&](const std::string& material_name, Asset_ref<Asset_texture_base> in_texture, Material_blend_mode in_blend_mode, bool in_instanced) -> Asset_ref<Asset_material>
	{
		auto ref = assetsystem_state->create_asset<Asset_material>(material_name);
		std::string vertex_shader = "content/engine/materials/default_ui.vert";
		std::string fragment_shader = "content/engine/materials/default_ui.frag";

		if (in_instanced)
		{
			vertex_shader = "content/engine/materials/default_ui_instanced.vert";
			fragment_shader = "content/engine/materials/default_ui_instanced.frag";
		}

		assetsystem_state->modify_asset<Asset_material>
		(
			ref,
			[&](Asset_material* material)
			{
				sic::import_material(*material, vertex_shader.c_str(), fragment_shader.c_str());

				material->m_blend_mode = in_blend_mode;

				Material_data_layout layout;
				layout.add_entry_vec4("lefttop_rightbottom_packed", { 0.0f, 0.0f, 0.0f, 0.0f });
				layout.add_entry_texture("uniform_texture", in_texture);

				material->initialize(1024, layout);
				material->set_is_instanced(in_instanced);
			}
		);

		return ref;
	};

	auto create_border_ui_material = [&](const std::string& material_name) -> Asset_ref<Asset_material>
	{
		auto ref = assetsystem_state->create_asset<Asset_material>(material_name);
		const std::string vertex_shader = "content/engine/materials/default_ui_instanced.vert";
		const std::string fragment_shader = "content/engine/materials/default_ui_border_instanced.frag";

		assetsystem_state->modify_asset<Asset_material>
		(
			ref,
			[&](Asset_material* material)
			{
				sic::import_material(*material, vertex_shader.c_str(), fragment_shader.c_str());
				material->m_blend_mode = Material_blend_mode::Translucent;

				Material_data_layout layout;
				layout.add_entry_vec4("lefttop_rightbottom_packed", { 0.0f, 0.0f, 0.0f, 0.0f });
				layout.add_entry_vec4("tint", editor_resources.m_background_color_darkest);

				material->initialize(1024, layout);
				material->set_is_instanced(true);
			}
		);

		return ref;
	};

	auto create_test_ui_text_material = [&](const std::string& material_name, Asset_ref<Asset_font> in_font) -> Asset_ref<Asset_material>
	{
		auto ref = assetsystem_state->create_asset<Asset_material>(material_name);
		std::string vertex_shader = "content/engine/materials/default_ui_instanced.vert";
		std::string fragment_shader = "content/engine/materials/default_ui_text_instanced.frag";

		assetsystem_state->modify_asset<Asset_material>
			(
				ref,
				[&](Asset_material* material)
				{
					sic::import_material(*material, vertex_shader.c_str(), fragment_shader.c_str());

					material->m_blend_mode = Material_blend_mode::Translucent;

					Material_data_layout layout;
					layout.add_entry_vec4("lefttop_rightbottom_packed", { 0.0f, 0.0f, 0.0f, 0.0f });
					layout.add_entry_texture("msdf", in_font.as<Asset_texture_base>());
					layout.add_entry_vec4("offset_and_size", { 0.0f, 0.0f, 0.0f, 0.0f });
					layout.add_entry_vec4("fg_color", { 1.0f, 1.0f, 1.0f, 1.0f });
					layout.add_entry_vec4("bg_color", { 0.0f, 0.0f, 0.0f, 0.0f });

					material->initialize(1024, layout);
					material->set_is_instanced(true);
				}
		);

		return ref;
	};

	editor_resources.m_default_font = assetsystem_state->create_asset<Asset_font>("test_font");
	{
		assetsystem_state->modify_asset<Asset_font>
		(
			editor_resources.m_default_font,
			[&](Asset_font* font)
			{
				Asset_font::Import_configuration conf;
				sic::import_font(*font, "content/fonts/arial.ttf", conf);
			}
		);
	}

	editor_resources.m_default_text_material = create_test_ui_text_material("ui_material_game_view", editor_resources.m_default_font);

	auto tex_sic_icon_small = create_test_texture("tex_sic_icon_small", "content/engine/editor/textures/t_sic_icon_small.png", true);
	auto tex_minimize_icon = create_test_texture("tex_minimize_icon", "content/engine/editor/textures/t_toolbar_icon_minimize.png", true);
	auto tex_maximize_icon = create_test_texture("tex_maximize_icon", "content/engine/editor/textures/t_toolbar_icon_maximize.png", true);
	auto tex_close_icon = create_test_texture("tex_close_icon", "content/engine/editor/textures/t_toolbar_icon_close.png", true);

	editor_resources.m_default_material = create_border_ui_material("ui_toolbar_material");
	editor_resources.m_sic_icon_material = create_test_ui_material("ui_sic_icon_mat", tex_sic_icon_small.as<Asset_texture_base>(), Material_blend_mode::Translucent, true);

	editor_resources.m_minimize_icon_material = create_test_ui_material("ui_minimize_icon_mat", tex_minimize_icon.as<Asset_texture_base>(), Material_blend_mode::Translucent, true);
	editor_resources.m_maximize_icon_material = create_test_ui_material("ui_maximize_icon_mat", tex_maximize_icon.as<Asset_texture_base>(), Material_blend_mode::Translucent, true);
	editor_resources.m_close_icon_material = create_test_ui_material("ui_close_icon_mat", tex_close_icon.as<Asset_texture_base>(), Material_blend_mode::Translucent, true);
}

void sic::editor::create_editor_window(Processor_editor_window in_processor, const std::string& in_name, const glm::ivec2& in_dimensions)
{
	in_processor.get_state_checked_w<State_window>().create_window(in_processor, in_name, in_dimensions, false);

	in_processor.schedule
	(
		[in_name, in_context = Engine_context(*in_processor.m_engine)](Engine_processor<Processor_flag_write<State_ui>> in_processor)
		{
			auto& ui_state = in_processor.get_state_checked_w<State_ui>();

			ui_state.bind<Ui_widget_base::On_clicked>
			(
				in_name + "_top_toolbar_button_minimize",
				[=](sic::Engine_processor<>) mutable
				{
					in_context.get_state_checked<State_window>().minimize_window(Processor_window(in_context), in_name);
				}
			);

			ui_state.bind<Ui_widget_base::On_clicked>
			(
				in_name + "_top_toolbar_button_maximize",
				[=](sic::Engine_processor<>) mutable
				{
					in_context.get_state_checked<State_window>().toggle_maximize_window(Processor_window(in_context), in_name);
				}
			);

			ui_state.bind<Ui_widget_base::On_clicked>
			(
				in_name + "_top_toolbar_button_close",
				[=](sic::Engine_processor<>) mutable
				{
					in_context.get_state_checked<State_window>().destroy_window(Processor_window(in_context), in_name);
				}
			);

			ui_state.bind<Ui_widget_base::On_drag>
			(
				in_name + "_top_toolbar",
				[=](sic::Engine_processor<>, const glm::vec2&, const glm::vec2&) mutable
				{
					State_window& window_state = in_context.get_state_checked<State_window>();
					window_state.begin_drag_window(Processor_window(in_context), in_name);
				}
			);

			ui_state.bind<Ui_widget_base::On_released>
			(
				in_name + "_top_toolbar",
				[=](sic::Engine_processor<>) mutable
				{
					State_window& window_state = in_context.get_state_checked<State_window>();
					window_state.end_drag_window(Processor_window(in_context), in_name);
				}
			);

			auto bind_border_events =
			[&ui_state, in_context, &in_name](const std::string& in_border_key, const glm::vec2& in_border_edge, const char* in_cursor_key) mutable
			{
				ui_state.bind<Ui_widget_base::On_drag>
				(
					in_border_key,
					[=](sic::Engine_processor<>, const glm::vec2&, const glm::vec2&) mutable
					{
						State_window& window_state = in_context.get_state_checked<State_window>();
						window_state.begin_resize(Processor_window(in_context), in_name, in_border_edge);
					}
				);

				ui_state.bind<Ui_widget_base::On_released>
				(
					in_border_key,
					[=](sic::Engine_processor<>) mutable
					{
						State_window& window_state = in_context.get_state_checked<State_window>();
						window_state.end_resize(Processor_window(in_context), in_name);
					}
				);

				ui_state.bind<Ui_widget_base::On_cursor_move_over>
				(
					in_border_key,
					[=](sic::Engine_processor<>) mutable
					{
						State_window& window_state = in_context.get_state_checked<State_window>();
						window_state.set_cursor(Processor_window(in_context), in_name, in_cursor_key);
					}
				);

				ui_state.bind<Ui_widget_base::On_pressed>
				(
					in_border_key,
					[=](sic::Engine_processor<>) mutable
					{
						State_window& window_state = in_context.get_state_checked<State_window>();
						window_state.set_cursor(Processor_window(in_context), in_name, in_cursor_key);
					}
				);

				ui_state.bind<Ui_widget_base::On_hover_end>
				(
					in_border_key,
					[=](sic::Engine_processor<>) mutable
					{
						State_window& window_state = in_context.get_state_checked<State_window>();
						window_state.set_cursor(Processor_window(in_context), in_name, "arrow");
					}
				);
			};

			bind_border_events(in_name + "_left_side_resize_border", { -1.0f, 0.0f }, "resize_ew");
			bind_border_events(in_name + "_right_side_resize_border", { 1.0f, 0.0f }, "resize_ew");
			bind_border_events(in_name + "_top_side_resize_border", { 0.0f, -1.0f }, "resize_ns");
			bind_border_events(in_name + "_bottom_side_resize_border", { 0.0f, 1.0f }, "resize_ns");

			bind_border_events(in_name + "_top_left_side_resize_border", { -1.0f, -1.0f }, "resize_nwse");
			bind_border_events(in_name + "_bottom_left_side_resize_border", { -1.0f, 1.0f }, "resize_nesw");
			bind_border_events(in_name + "_top_right_side_resize_border", { 1.0f, -1.0f }, "resize_nesw");
			bind_border_events(in_name + "_bottom_right_side_resize_border", { 1.0f, 1.0f }, "resize_nwse");

			Ui_widget_canvas* canvas = ui_state.find<Ui_widget_canvas>(in_name);
			assert(canvas);

			const common::State_editor_ui_resources& editor_resources = in_context.get_state_checked<common::State_editor_ui_resources>();

			const float resize_border_size = 8.0f;
			const glm::vec4 border_color = editor_resources.m_background_color_darkest;

			canvas->add_child
			(
				Ui_slot_canvas()
				.anchors(Ui_anchors(0.0f, 0.0f, 1.0f, 1.0f))
				.pivot({ 0.5f, 0.5f })
				.size(canvas->m_reference_dimensions - glm::vec2(1.0f, 1.0f))
				.translation(glm::vec2(0.0f, 0.0f))
				.rotation(0.0f),

				ui_state.create<Ui_widget_canvas>(in_name + "_content_area")
				.reference_dimensions({ 1920.0f, 1080.0f })
			);

			canvas->add_child
			(
				Ui_slot_canvas()
				.anchors(Ui_anchors(0.0f, 0.0f, 0.0f, 1.0f))
				.pivot({ 0.0f, 0.5f })
				.size({ 1.0f, canvas->m_reference_dimensions.y })
				.translation(glm::vec2(0.0f, 0.0f))
				.rotation(0.0f),

				ui_state.create<Ui_widget_image>(in_name + "_left_border")
				.material(editor_resources.m_default_material)
				.tint(border_color)
			);

			canvas->add_child
			(
				Ui_slot_canvas()
				.anchors(Ui_anchors(1.0f, 0.0f, 1.0f, 1.0f))
				.pivot({ 1.0f, 0.5f })
				.size({ 1.0f, canvas->m_reference_dimensions.y })
				.translation(glm::vec2(0.0f, 0.0f))
				.rotation(0.0f),

				ui_state.create<Ui_widget_image>(in_name + "_right_border")
				.material(editor_resources.m_default_material)
				.tint(border_color)
			);

			canvas->add_child
			(
				Ui_slot_canvas()
				.anchors(Ui_anchors(0.0f, 0.0f, 1.0f, 0.0f))
				.pivot({ 0.5f, 0.0f })
				.size({ canvas->m_reference_dimensions.x , 1.0f })
				.translation(glm::vec2(0.0f, 0.0f))
				.rotation(0.0f),

				ui_state.create<Ui_widget_image>(in_name + "_top_border")
				.material(editor_resources.m_default_material)
				.tint(border_color)
			);

			canvas->add_child
			(
				Ui_slot_canvas()
				.anchors(Ui_anchors(0.0f, 1.0f, 1.0f, 1.0f))
				.pivot({ 0.5f, 1.0f })
				.size({ canvas->m_reference_dimensions.x , 1.0f })
				.translation(glm::vec2(0.0f, 0.0f))
				.rotation(0.0f),

				ui_state.create<Ui_widget_image>(in_name + "_bottom_border")
				.material(editor_resources.m_default_material)
				.tint(border_color)
			);

			canvas->add_child
			(
				Ui_slot_canvas()
				.anchors(Ui_anchors(0.0f, 0.0f, 1.0f, 0.0f))
				.pivot({ 0.5f, 0.0f })
				.translation({ 0.0f, 0.0f })
				.size({ canvas->m_reference_dimensions.x, 32.0f }),

				ui_state.create<Ui_widget_image>(in_name + "_top_toolbar")
				.image_size({ canvas->m_reference_dimensions.x, 32.0f })
				.material(editor_resources.m_default_material)
				.tint(border_color)
			);

			canvas->add_child
			(
				Ui_slot_canvas()
				.anchors(Ui_anchors(0.0f, 0.0f))
				.pivot({ 0.0f, 0.0f })
				.translation({ 4.0f, 0.0f })
				.size({ 32.0f, 32.0f }),

				ui_state.create<Ui_widget_image>(in_name + "_top_toolbar_sic_icon")
				.image_size({ 32.0f, 32.0f })
				.material(editor_resources.m_sic_icon_material)
			);

			canvas->add_child
			(
				Ui_slot_canvas()
				.anchors(Ui_anchors(1.0f, 0.0f))
				.pivot({ 1.0f, 0.5f })
				.translation({ 0.0f, 32.0f * 0.5f }) //half the toolbar height
				.autosize(true),

				ui_state.create<Ui_widget_horizontal_box>(in_name + "_top_toolbar_button_container")
				.add_child
				(
					Ui_slot_horizontal_box().v_align(Ui_v_alignment::top),
					ui_state.create<Ui_widget_button>(in_name + "_top_toolbar_button_minimize")
					.size({ 30.0f, 32.0f })
					.base_material(editor_resources.m_default_material)
					.tints({ 0.0f, 0.0f, 0.0f, 0.0f }, editor_resources.m_foreground_color_darker, editor_resources.m_foreground_color_dark)

					.add_child
					(
						Ui_slot_button(),
						ui_state.create<Ui_widget_image>(in_name + "_top_toolbar_button_minimize_image")
						.image_size({ 16.0f, 16.0f })
						.material(editor_resources.m_minimize_icon_material)
						.interaction_consume(Interaction_consume::fall_through)
					)
				)
				.add_child
				(
					Ui_slot_horizontal_box().v_align(Ui_v_alignment::top),
					ui_state.create<Ui_widget_button>(in_name + "_top_toolbar_button_maximize")
					.size({ 30.0f, 32.0f })
					.base_material(editor_resources.m_default_material)
					.tints({ 0.0f, 0.0f, 0.0f, 0.0f }, editor_resources.m_foreground_color_darker, editor_resources.m_foreground_color_dark)

					.add_child
					(
						Ui_slot_button(),
						ui_state.create<Ui_widget_image>(in_name + "_top_toolbar_button_maxmimize_image")
						.image_size({ 16.0f, 16.0f })
						.material(editor_resources.m_maximize_icon_material)
						.interaction_consume(Interaction_consume::fall_through)
					)
				)
				.add_child
				(
					Ui_slot_horizontal_box().v_align(Ui_v_alignment::top),
					ui_state.create<Ui_widget_button>(in_name + "_top_toolbar_button_close")
					.size({ 30.0f, 32.0f })
					.base_material(editor_resources.m_default_material)
					.tints({ 0.0f, 0.0f, 0.0f, 0.0f }, editor_resources.m_error_color_light, editor_resources.m_error_color_medium)

					.add_child
					(
						Ui_slot_button(),
						ui_state.create<Ui_widget_image>(in_name + "_top_toolbar_button_close_image")
						.image_size({ 16.0f, 16.0f })
						.material(editor_resources.m_close_icon_material)
						.interaction_consume(Interaction_consume::fall_through)
					)
				)
				.add_child
				(
					Ui_slot_horizontal_box().v_align(Ui_v_alignment::center),
					sic::editor::common::create_toggle_box(ui_state, editor_resources, in_name + "_test_toggle1")
				)
				.add_child
				(
					Ui_slot_horizontal_box().v_align(Ui_v_alignment::center),
					sic::editor::common::create_toggle_box(ui_state, editor_resources, in_name + "_test_toggle2")
				)
				.add_child
				(
					Ui_slot_horizontal_box().v_align(Ui_v_alignment::center),
					sic::editor::common::create_toggle_box(ui_state, editor_resources, in_name + "_test_toggle3")
				)
				.add_child
				(
					Ui_slot_horizontal_box().v_align(Ui_v_alignment::center),
					sic::editor::common::create_toggle_box(ui_state, editor_resources, in_name + "_test_toggle4")
				)
				.add_child
				(
					Ui_slot_horizontal_box().v_align(Ui_v_alignment::center),
					sic::editor::common::create_toggle_box(ui_state, editor_resources, in_name + "_test_toggle5")
				)
				.add_child
				(
					Ui_slot_horizontal_box().v_align(Ui_v_alignment::center),
					sic::editor::common::create_input_box(ui_state, editor_resources, in_name + "_test_input")
				)
				.add_child
				(
					Ui_slot_horizontal_box().v_align(Ui_v_alignment::center),
					sic::editor::common::create_toggle_slider(ui_state, editor_resources, in_name + "_test_toggle6")
				)
			);

			canvas->add_child
			(
				Ui_slot_canvas()
				.anchors(Ui_anchors(0.0f, 0.0f, 0.0f, 1.0f))
				.pivot({ 0.0f, 0.5f })
				.size({ resize_border_size, canvas->m_reference_dimensions.y })
				.translation(glm::vec2(0.0f, 0.0f))
				.rotation(0.0f),

				ui_state.create<Ui_widget_image>(in_name + "_left_side_resize_border")
			);

			canvas->add_child
			(
				Ui_slot_canvas()
				.anchors(Ui_anchors(1.0f, 0.0f, 1.0f, 1.0f))
				.pivot({ 1.0f, 0.5f })
				.size({ resize_border_size, canvas->m_reference_dimensions.y })
				.translation(glm::vec2(0.0f, 0.0f))
				.rotation(0.0f),

				ui_state.create<Ui_widget_image>(in_name + "_right_side_resize_border")
			);

			canvas->add_child
			(
				Ui_slot_canvas()
				.anchors(Ui_anchors(0.0f, 0.0f, 1.0f, 0.0f))
				.pivot({ 0.5f, 0.0f })
				.size({ canvas->m_reference_dimensions.x, resize_border_size })
				.translation(glm::vec2(0.0f, 0.0f))
				.rotation(0.0f),

				ui_state.create<Ui_widget_image>(in_name + "_top_side_resize_border")
			);

			canvas->add_child
			(
				Ui_slot_canvas()
				.anchors(Ui_anchors(0.0f, 1.0f, 1.0f, 1.0f))
				.pivot({ 0.5f, 1.0f })
				.size({ canvas->m_reference_dimensions.x, resize_border_size })
				.translation(glm::vec2(0.0f, 0.0f))
				.rotation(0.0f),

				ui_state.create<Ui_widget_image>(in_name + "_bottom_side_resize_border")
			);

			canvas->add_child
			(
				Ui_slot_canvas()
				.anchors(Ui_anchors(0.0f, 0.0f, 0.0f, 0.0f))
				.pivot({ 0.0f, 0.0f })
				.size({ resize_border_size, resize_border_size })
				.translation(glm::vec2(0.0f, 0.0f))
				.rotation(0.0f),

				ui_state.create<Ui_widget_image>(in_name + "_top_left_side_resize_border")
			);

			canvas->add_child
			(
				Ui_slot_canvas()
				.anchors(Ui_anchors(0.0f, 1.0f, 0.0f, 1.0f))
				.pivot({ 0.0f, 1.0f })
				.size({ resize_border_size, resize_border_size })
				.translation(glm::vec2(0.0f, 0.0f))
				.rotation(0.0f),

				ui_state.create<Ui_widget_image>(in_name + "_bottom_left_side_resize_border")
			);

			canvas->add_child
			(
				Ui_slot_canvas()
				.anchors(Ui_anchors(1.0f, 0.0f, 1.0f, 0.0f))
				.pivot({ 1.0f, 0.0f })
				.size({ resize_border_size, resize_border_size })
				.translation(glm::vec2(0.0f, 0.0f))
				.rotation(0.0f),

				ui_state.create<Ui_widget_image>(in_name + "_top_right_side_resize_border")
			);

			canvas->add_child
			(
				Ui_slot_canvas()
				.anchors(Ui_anchors(1.0f, 1.0f, 1.0f, 1.0f))
				.pivot({ 1.0f, 1.0f })
				.size({ resize_border_size, resize_border_size })
				.translation(glm::vec2(0.0f, 0.0f))
				.rotation(0.0f),

				ui_state.create<Ui_widget_image>(in_name + "_bottom_right_side_resize_border")
			);
		}
	);
}
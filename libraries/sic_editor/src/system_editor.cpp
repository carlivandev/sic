#include "sic/editor/system_editor.h"

#include "sic/editor/asset_import.h"

#include "sic/core/engine_context.h"

#include "sic/system_asset.h"
#include "sic/asset_texture.h"
#include "sic/system_ui.h"

void sic::editor::System_editor::on_created(Engine_context in_context)
{
	in_context.register_state<State_editor>("State_editor");
}

void sic::editor::System_editor::on_engine_finalized(Engine_context in_context) const
{
	State_editor& editor_state = in_context.get_state_checked<State_editor>();
	State_assetsystem* assetsystem_state = in_context.get_state<State_assetsystem>();

	auto create_test_texture = [&](const std::string& texture_name, const std::string& texture_source, bool in_is_ui_image = false) -> Asset_ref<Asset_texture>
	{
		auto ref = assetsystem_state->create_asset<Asset_texture>(texture_name, "content/textures/crysis");
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

		assetsystem_state->save_asset<Asset_texture>(ref);

		return ref;
	};

	auto create_test_ui_material = [&](const std::string& material_name, Asset_ref<Asset_texture_base> in_texture, Material_blend_mode in_blend_mode, bool in_instanced) -> Asset_ref<Asset_material>
	{
		auto ref = assetsystem_state->create_asset<Asset_material>(material_name, "content/materials/ui");
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
				material->set_texture_parameter("uniform_texture", in_texture);

				material->m_blend_mode = in_blend_mode;

				Material_data_layout layout;
				layout.add_entry<glm::vec4>("lefttop_rightbottom_packed");
				layout.add_entry<GLuint64>("uniform_texture");

				material->initialize(1024, layout);
				material->set_is_instanced(in_instanced);
			}
		);

		assetsystem_state->save_asset(ref);

		return ref;
	};

	auto create_border_ui_material = [&](const std::string& material_name) -> Asset_ref<Asset_material>
	{
		auto ref = assetsystem_state->create_asset<Asset_material>(material_name, "content/materials/ui");
		const std::string vertex_shader = "content/engine/materials/default_ui_instanced.vert";
		const std::string fragment_shader = "content/engine/materials/default_ui_border_instanced.frag";

		assetsystem_state->modify_asset<Asset_material>
		(
			ref,
			[&](Asset_material* material)
			{
				sic::import_material(*material, vertex_shader.c_str(), fragment_shader.c_str());

				material->set_vec4_parameter("tint", glm::vec4(23.0f, 23.0f, 23.0f, 255.0f) / 255.0f);

				material->m_blend_mode = Material_blend_mode::Translucent;

				Material_data_layout layout;
				layout.add_entry<glm::vec4>("lefttop_rightbottom_packed");
				layout.add_entry<glm::vec4>("tint");

				material->initialize(1024, layout);
				material->set_is_instanced(true);
			}
		);

		assetsystem_state->save_asset(ref);

		return ref;
	};

	auto tex_sic_icon_small = create_test_texture("tex_sic_icon_small", "content/engine/editor/textures/t_sic_icon_small.png", true);
	auto tex_minimize_icon = create_test_texture("tex_minimize_icon", "content/engine/editor/textures/t_toolbar_icon_minimize.png", true);
	auto tex_maximize_icon = create_test_texture("tex_maximize_icon", "content/engine/editor/textures/t_toolbar_icon_maximize.png", true);
	auto tex_close_icon = create_test_texture("tex_close_icon", "content/engine/editor/textures/t_toolbar_icon_close.png", true);

	editor_state.m_toolbar_material = create_border_ui_material("ui_toolbar_material");
	editor_state.m_sic_icon_material = create_test_ui_material("ui_sic_icon_mat", tex_sic_icon_small.as<Asset_texture_base>(), Material_blend_mode::Translucent, true);

	editor_state.m_minimize_icon_material = create_test_ui_material("ui_minimize_icon_mat", tex_minimize_icon.as<Asset_texture_base>(), Material_blend_mode::Translucent, true);
	editor_state.m_maximize_icon_material = create_test_ui_material("ui_maximize_icon_mat", tex_maximize_icon.as<Asset_texture_base>(), Material_blend_mode::Translucent, true);
	editor_state.m_close_icon_material = create_test_ui_material("ui_close_icon_mat", tex_close_icon.as<Asset_texture_base>(), Material_blend_mode::Translucent, true);
}

void sic::editor::create_editor_window(Processor_editor_window in_processor, const std::string& in_name, const glm::ivec2& in_dimensions)
{
	in_processor.get_state_checked_w<State_window>().create_window(in_processor, in_name, in_dimensions);

	in_processor.update_state_deferred<State_ui>
	(
		[in_processor, in_name, in_context = Engine_context(*in_processor.m_engine)](State_ui& ui_state)
		{
			ui_state.bind<Ui_widget::On_clicked>
			(
				in_name + "_top_toolbar_button_minimize",
				[=]() mutable
				{
					in_context.get_state_checked<State_window>().minimize_window(Processor_window(in_context), in_name);
				}
			);

			ui_state.bind<Ui_widget::On_clicked>
			(
				in_name + "_top_toolbar_button_maximize",
				[=]() mutable
				{
					in_context.get_state_checked<State_window>().toggle_maximize_window(Processor_window(in_context), in_name);
				}
			);

			ui_state.bind<Ui_widget::On_clicked>
			(
				in_name + "_top_toolbar_button_close",
				[=]() mutable
				{
					in_context.get_state_checked<State_window>().destroy_window(Processor_window(in_context), in_name);
				}
			);

			ui_state.bind<Ui_widget::On_drag>
			(
				in_name + "_top_toolbar",
				[=](const glm::vec2&, const glm::vec2&) mutable
				{
					State_window& window_state = in_context.get_state_checked<State_window>();
					window_state.begin_drag_window(Processor_window(in_context), in_name);
				}
			);

			ui_state.bind<Ui_widget::On_released>
			(
				in_name + "_top_toolbar",
				[=]() mutable
				{
					State_window& window_state = in_context.get_state_checked<State_window>();
					window_state.end_drag_window(Processor_window(in_context), in_name);
				}
			);

			auto bind_border_events =
			[&ui_state, in_context, &in_name](const std::string& in_border_key, const glm::vec2& in_border_edge, const char* in_cursor_key) mutable
			{
				ui_state.bind<Ui_widget::On_drag>
				(
					in_border_key,
					[=](const glm::vec2&, const glm::vec2&) mutable
					{
						State_window& window_state = in_context.get_state_checked<State_window>();
						window_state.begin_resize(Processor_window(in_context), in_name, in_border_edge);
					}
				);

				ui_state.bind<Ui_widget::On_released>
				(
					in_border_key,
					[=]() mutable
					{
						State_window& window_state = in_context.get_state_checked<State_window>();
						window_state.end_resize(Processor_window(in_context), in_name);
					}
				);

				ui_state.bind<Ui_widget::On_cursor_move_over>
				(
					in_border_key,
					[=]() mutable
					{
						State_window& window_state = in_context.get_state_checked<State_window>();
						window_state.set_cursor(Processor_window(in_context), in_name, in_cursor_key);
					}
				);

				ui_state.bind<Ui_widget::On_pressed>
				(
					in_border_key,
					[=]() mutable
					{
						State_window& window_state = in_context.get_state_checked<State_window>();
						window_state.set_cursor(Processor_window(in_context), in_name, in_cursor_key);
					}
				);

				ui_state.bind<Ui_widget::On_hover_end>
				(
					in_border_key,
					[=]() mutable
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

			const State_editor& editor_state = in_context.get_state_checked<State_editor>();

			const float resize_border_size = 8.0f;
			const glm::vec4 border_color = glm::vec4(23.0f, 23.0f, 23.0f, 255.0f) / 255.0f;

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
				.material(editor_state.m_toolbar_material)
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
				.material(editor_state.m_toolbar_material)
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
				.material(editor_state.m_toolbar_material)
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
				.material(editor_state.m_toolbar_material)
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
				.material(editor_state.m_toolbar_material)
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
				.material(editor_state.m_sic_icon_material)
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
					.base_material(editor_state.m_toolbar_material)
					.tints({ 0.0f, 0.0f, 0.0f, 0.0f }, glm::vec4(84.0f, 84.0f, 84.0f, 255.0f) / 255.0f, glm::vec4(150.0f, 150.0f, 150.0f, 255.0f) / 255.0f)

					.add_child
					(
						Ui_slot_button(),
						ui_state.create<Ui_widget_image>(in_name + "_top_toolbar_button_minimize_image")
						.image_size({ 16.0f, 16.0f })
						.material(editor_state.m_minimize_icon_material)
						.interaction_consume(Interaction_consume::fall_through)
					)
				)
				.add_child
				(
					Ui_slot_horizontal_box().v_align(Ui_v_alignment::top),
					ui_state.create<Ui_widget_button>(in_name + "_top_toolbar_button_maximize")
					.size({ 30.0f, 32.0f })
					.base_material(editor_state.m_toolbar_material)
					.tints({ 0.0f, 0.0f, 0.0f, 0.0f }, glm::vec4(84.0f, 84.0f, 84.0f, 255.0f) / 255.0f, glm::vec4(150.0f, 150.0f, 150.0f, 255.0f) / 255.0f)

					.add_child
					(
						Ui_slot_button(),
						ui_state.create<Ui_widget_image>(in_name + "_top_toolbar_button_maxmimize_image")
						.image_size({ 16.0f, 16.0f })
						.material(editor_state.m_maximize_icon_material)
						.interaction_consume(Interaction_consume::fall_through)
					)
				)
				.add_child
				(
					Ui_slot_horizontal_box().v_align(Ui_v_alignment::top),
					ui_state.create<Ui_widget_button>(in_name + "_top_toolbar_button_close")
					.size({ 30.0f, 32.0f })
					.base_material(editor_state.m_toolbar_material)
					.tints({ 0.0f, 0.0f, 0.0f, 0.0f }, { 1.0f, 0.5f, 0.5f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f })

					.add_child
					(
						Ui_slot_button(),
						ui_state.create<Ui_widget_image>(in_name + "_top_toolbar_button_close_image")
						.image_size({ 16.0f, 16.0f })
						.material(editor_state.m_close_icon_material)
						.interaction_consume(Interaction_consume::fall_through)
					)
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

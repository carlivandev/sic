#include "pch.h"

#include "sic/engine_context.h"
#include "sic/level_context.h"

#include "sic/system_window.h"
#include "sic/system_input.h"
#include "sic/system_editor_view_controller.h"
#include "sic/system_model.h"
#include "sic/system_file.h"
#include "sic/system_asset.h"
#include "sic/system_renderer.h"
#include "sic/system_renderer_state_swapper.h"
#include "sic/system_ui.h"

#include "sic/object.h"
#include "sic/system_view.h"
#include "sic/component_transform.h"
#include "sic/file_management.h"
#include "sic/asset_types.h"
#include "sic/leavable_queue.h"
#include "sic/logger.h"
#include "sic/debug_draw_helpers.h"
#include "sic/weak_ref.h"
#include "sic/material_dynamic_parameters.h"

#include "sic/shader_parser.h"
#include <type_traits>

using namespace sic;

struct System_game : sic::System
{
	struct Component_player_move : Component
	{
		float m_t = 0.0f;
		Material_dynamic_parameters m_parameters;
	};

	struct Player : Object<Player, Component_transform, Component_model, Component_player_move> {};

	void on_created(Engine_context in_context) override
	{
		in_context.register_component_type<Component_transform>("transform");
		in_context.register_component_type<Component_player_move>("Component_player_move");
		in_context.register_object<Player>("Player");

		in_context.create_subsystem<System_editor_view_controller>(*this);
	}

	void on_engine_finalized(Engine_context in_context) const override
	{
		in_context.create_level(nullptr);

		State_window* window_state = in_context.get_state<State_window>();
		window_state->create_window(in_context, "main_window", { 1600, 800 });
		window_state->create_window(in_context, "second_window", { 1600, 800 });
	}

	void on_begin_simulation(Scene_context in_context) const override
	{
		State_window* window_state = in_context.get_engine_context().get_state<State_window>();
		auto main_window = window_state->find_window("main_window");
		auto secondary_window = window_state->find_window("second_window");

		Component_view& main_view = in_context.create_object<Object_view>().get<Component_view>();
		main_view.set_clear_color({ 0.0f, 0.0f, 0.1f, 1.0f });
		main_view.set_viewport_size({ 0.5f, 0.5f });
		main_view.set_viewport_dimensions(main_window->get_dimensions());
		main_view.set_window(main_window);

		main_view.set_near_and_far_plane(10.0f, 50000.0f);

		Component_view& secondary_view = in_context.create_object<Object_view>().get<Component_view>();
		secondary_view.set_clear_color({ 0.0f, 0.0f, 0.1f, 1.0f });
		secondary_view.set_window(secondary_window);
		secondary_view.set_viewport_size({ 0.5f, 0.5f });
		secondary_view.set_viewport_offset({ 0.5f, 0.5f });
		secondary_view.get_owner().find<Component_transform>()->set_translation({ -400.0f, 0.0f, 0.0f });
		secondary_view.get_owner().find<Component_transform>()->look_at(glm::normalize(glm::vec3(50.0f, 50.0f, -200.0f) - glm::vec3{ -400.0f, 0.0f, 0.0f }), Transform::up);
		secondary_view.set_viewport_dimensions(secondary_window->get_dimensions());

		in_context.create_object<Object_editor_view_controller>().get<Component_editor_view_controller>().m_view_to_control = &main_view;

		State_assetsystem* assetsystem_state = in_context.get_engine_context().get_state<State_assetsystem>();

#if 1
		auto create_test_texture = [&](const std::string& texture_name, const std::string& texture_source) -> Asset_ref<Asset_texture>
		{
			auto ref = assetsystem_state->create_asset<Asset_texture>(texture_name, "content/textures/crysis");
			assetsystem_state->modify_asset<Asset_texture>
			(
				ref,
				[&texture_source](Asset_texture* texture)
				{
					texture->import(File_management::load_file(texture_source), true);
				}
			);

			assetsystem_state->save_asset<Asset_texture>(*ref.get_header());

			return ref;
		};

		auto create_test_master_material = [&](const std::string& material_name, Material_blend_mode in_blend_mode, bool in_instanced) -> Asset_ref<Asset_material>
		{
			auto ref = assetsystem_state->create_asset<Asset_material>(material_name, "content/materials/crysis");
			std::string vertex_shader = "content/materials/material.vert";
			std::string fragment_shader = in_blend_mode == Material_blend_mode::Opaque ? "content/materials/material.frag" : "content/materials/material_translucent.frag";

			if (in_instanced)
			{
				vertex_shader = "content/materials/material_instanced.vert";
				fragment_shader = in_blend_mode == Material_blend_mode::Opaque ? "content/materials/material_instanced.frag" : "content/materials/material_translucent_instanced.frag";
			}

			assetsystem_state->modify_asset<Asset_material>
				(
					ref,
					[&](Asset_material* material)
					{
						material->import(vertex_shader.c_str(), fragment_shader.c_str());
						if (in_blend_mode == Material_blend_mode::Opaque)
						{
							material->set_texture_parameter("tex_albedo", Asset_ref<Asset_texture>());
							material->set_vec4_parameter("color_additive", {0.0f, 0.0f, 0.0f, 0.0f});
						}

						material->m_blend_mode = in_blend_mode;

						Material_data_layout layout;
						layout.add_entry<glm::mat4x4>("MVP");
						layout.add_entry<glm::mat4x4>("model_matrix");
						layout.add_entry<GLuint64>("tex_albedo");
						layout.add_entry<glm::vec4>("color_additive");

						material->initialize(1024, layout);
						material->set_is_instanced(in_instanced);
					}
			);

			assetsystem_state->save_asset<Asset_material>(*ref.get_header());

			return ref;
		};

		auto create_test_child_material = [&](const std::string& material_name, Asset_ref<Asset_texture> in_texture, Asset_ref<Asset_material> in_parent) -> Asset_ref<Asset_material>
		{
			auto ref = assetsystem_state->create_asset<Asset_material>(material_name, "content/materials/crysis");
			assetsystem_state->modify_asset<Asset_material>
			(
				ref,
				[&](Asset_material* material)
				{
					material->set_parent(in_parent.get_mutable());
					material->set_texture_parameter("tex_albedo", in_texture);
				}
			);

			assetsystem_state->save_asset<Asset_material>(*ref.get_header());

			return ref;
		};

		auto create_test_material = [&](const std::string& material_name, Asset_ref<Asset_texture> in_texture, Material_blend_mode in_blend_mode, bool in_instanced) -> Asset_ref<Asset_material>
		{
			auto ref = assetsystem_state->create_asset<Asset_material>(material_name, "content/materials/crysis");
			std::string vertex_shader = "content/materials/material.vert";
			std::string fragment_shader = in_blend_mode == Material_blend_mode::Opaque ? "content/materials/material.frag" : "content/materials/material_translucent.frag";

			if (in_instanced)
			{
				vertex_shader = "content/materials/material_instanced.vert";
				fragment_shader = in_blend_mode == Material_blend_mode::Opaque ? "content/materials/material_instanced.frag" : "content/materials/material_translucent_instanced.frag";
			}

			assetsystem_state->modify_asset<Asset_material>
			(
				ref,
				[&](Asset_material* material)
				{
					material->import(vertex_shader.c_str(), fragment_shader.c_str());
					if (in_blend_mode == Material_blend_mode::Opaque)
						material->set_texture_parameter("tex_albedo", in_texture);

					material->m_blend_mode = in_blend_mode;

					Material_data_layout layout;
					layout.add_entry<glm::mat4x4>("MVP");
					layout.add_entry<glm::mat4x4>("model_matrix");
					layout.add_entry<GLuint64>("tex_albedo");

					material->initialize(1024, layout);
					material->set_is_instanced(in_instanced);
				}
			);

			assetsystem_state->save_asset<Asset_material>(*ref.get_header());

			return ref;
		};

		auto tex_body =		(create_test_texture("tex_crysis_body", "content/textures/crysis/body_dif.png"));
		auto tex_glass =	(create_test_texture("tex_crysis_glass", "content/textures/crysis/glass_dif.png"));
		auto tex_arm =		(create_test_texture("tex_crysis_arm", "content/textures/crysis/arm_dif.png"));
		auto tex_hand =		(create_test_texture("tex_crysis_hand", "content/textures/crysis/hand_dif.png"));
		auto tex_helmet =	(create_test_texture("tex_crysis_helmet", "content/textures/crysis/helmet_dif.png"));
		auto tex_leg =		(create_test_texture("tex_crysis_leg", "content/textures/crysis/leg_dif.png"));

		bool instancing = true;

		auto opaque_master_mat = (create_test_master_material("opaque_master_mat", Material_blend_mode::Opaque, instancing));


		//uncomment for no master material setup
// 		auto mat_body =		(create_test_material("mat_crysis_body", tex_body, Material_blend_mode::Opaque, instancing));
// 		auto mat_arm =		(create_test_material("mat_crysis_arm", tex_arm, Material_blend_mode::Opaque, instancing));
// 		auto mat_hand =		(create_test_material("mat_crysis_hand", tex_hand, Material_blend_mode::Opaque, instancing));
// 		auto mat_helmet =	(create_test_material("mat_crysis_helmet", tex_helmet, Material_blend_mode::Opaque, instancing));
// 		auto mat_leg =		(create_test_material("mat_crysis_leg", tex_leg, Material_blend_mode::Opaque, instancing));

 		auto mat_glass =	(create_test_material("mat_crysis_glass", tex_glass, Material_blend_mode::Additive, instancing));

		auto mat_body = (create_test_child_material("mat_crysis_body", tex_body, opaque_master_mat));
		auto mat_arm = (create_test_child_material("mat_crysis_arm", tex_arm, opaque_master_mat));
		auto mat_hand = (create_test_child_material("mat_crysis_hand", tex_hand, opaque_master_mat));
		auto mat_helmet = (create_test_child_material("mat_crysis_helmet", tex_helmet, opaque_master_mat));
		auto mat_leg = (create_test_child_material("mat_crysis_leg", tex_leg, opaque_master_mat));		

		Asset_ref<Asset_model> model_ref = assetsystem_state->create_asset<Asset_model>("model_crysis", "content/models");
		assetsystem_state->modify_asset<Asset_model>
		(
			model_ref,
			[&](Asset_model* model)
			{
				model->import(File_management::load_file("content/models/crysis.fbx"));
				model->set_material(mat_body, "Body");
				model->set_material(mat_glass, "Glass");
				model->set_material(mat_arm, "Arm");
				model->set_material(mat_hand, "Hand");
				model->set_material(mat_helmet, "Helmet");
				model->set_material(mat_leg, "Leg");
			}
		);
		

		assetsystem_state->save_asset<Asset_model>(*model_ref.get_header());
#else
		Asset_ref<Asset_model> model_ref = assetsystem_state->find_asset<Asset_model>(xg::Guid("46aae3ac-64f8-4db1-82a8-691378774aee"));
#endif

		

		{
			Player& new_player = in_context.create_object<Player>();
			Component_model& player_model = new_player.get<Component_model>();
			player_model.set_model(model_ref);

			new_player.get<Component_transform>().set_translation({ 50.0f, 50.0f, -200.0f });
			new_player.get<Component_transform>().set_scale({ 10.0f, 10.0f, 10.0f });

			new_player.get<Component_player_move>().m_parameters.bind(in_context, player_model);
		}

		{
			Player& new_player = in_context.create_object<Player>();
			Component_model& player_model = new_player.get<Component_model>();
			player_model.set_model(model_ref);

			new_player.get<Component_transform>().set_translation({ 200.0f, 0.0f, 0.0f });
			new_player.get<Component_transform>().look_at(glm::normalize(glm::vec3(50.0f, 50.0f, -200.0f) - glm::vec3(200.0f, 0.0f, 0.0f)), sic::Transform::up);
			new_player.get<Component_transform>().set_scale({ 10.0f, 20.0f, 10.0f });

			new_player.get<Component_player_move>().m_parameters.bind(in_context, player_model, "Leg");
			new_player.get<Component_player_move>().m_parameters.set_parameter(in_context, "tex_albedo", tex_glass);
		}

		for (size_t x = 0; x < 20; x++)
		{
			for (size_t y = 0; y < 40; y++)
			{
				Player& new_player = in_context.create_object<Player>();
				Component_model& player_model = new_player.get<Component_model>();
				player_model.set_model(model_ref);

				new_player.get<Component_transform>().set_translation({ x * 80.0f, y * 80.0f, 0.0f });
				new_player.get<Component_transform>().look_at(glm::normalize(glm::vec3(50.0f, 50.0f, -200.0f) - glm::vec3(200.0f, 0.0f, 0.0f)), sic::Transform::up);
				new_player.get<Component_transform>().set_scale({ 10.0f, 10.0f, 10.0f });
				new_player.get<Component_player_move>().m_t = (x + y * 0.5f) * 10.0f;
				new_player.get<Component_player_move>().m_parameters.bind(in_context, player_model, "Hand");

				assert(in_context.get_does_object_exist(new_player.get_id(), false));
			}
		}

		in_context.get_engine_context().get_state<State_filesystem>()->request_load
		(
			File_load_request
			(
				"content/models/crysis.fbx",
				[](std::string && in_data)
				{
					in_data;
					SIC_LOG(g_log_game, "Loaded!");
				}
			)
		);

		SIC_LOG(g_log_game, "ok");
		SIC_LOG_W(g_log_game, "hmmm!");
		SIC_LOG_E(g_log_game, "oh no!");
		SIC_LOG_V(g_log_game, "it works!");

		auto create_test_ui_material = [&](const std::string& material_name, Asset_ref<Asset_texture> in_texture, Material_blend_mode in_blend_mode, bool in_instanced) -> Asset_ref<Asset_material>
		{
			auto ref = assetsystem_state->create_asset<Asset_material>(material_name, "content/materials/ui");
			std::string vertex_shader = "content/engine/materials/default_ui.vert";
			std::string fragment_shader = "content/materials/material_ui_test.frag";

			if (in_instanced)
			{
				vertex_shader = "content/materials/pass_through_instanced.vert";
				fragment_shader = "content/materials/material_ui_test_instanced.frag";
			}

			assetsystem_state->modify_asset<Asset_material>
			(
				ref,
				[&](Asset_material* material)
				{
					material->import(vertex_shader.c_str(), fragment_shader.c_str());
					material->set_texture_parameter("uniform_texture", in_texture);

					material->m_blend_mode = in_blend_mode;

					Material_data_layout layout;
					layout.add_entry<GLuint64>("uniform_texture");
					layout.add_entry<glm::vec4>("lefttop_rightbottom_packed");

					material->initialize(1024, layout);
					material->set_is_instanced(in_instanced);
				}
			);

			assetsystem_state->save_asset<Asset_material>(*ref.get_header());

			return ref;
		};

		auto tex_ui_test = (create_test_texture("tex_ui_test", "content/textures/ui_test_image.png"));
		auto ui_mat = create_test_ui_material("ui_material_test", tex_ui_test, Material_blend_mode::Translucent, false);
		
 		State_ui& ui_state = in_context.get_engine_context().get_state_checked<State_ui>();

		ui_state.update
		(
			[ui_mat, &ui_state](Ui_context& inout_context)
			{
				Ui_widget_canvas* canvas = inout_context.find_widget<Ui_widget_canvas>("main_window");
				assert(canvas);

				Ui_slot_canvas canvas_slot;
				canvas_slot.m_anchors.set(0.5f, 0.5f);
				canvas_slot.m_pivot = { 1.0f, 1.0f };
				canvas_slot.m_size = { 256.0f, 256.0f }; //relative to parents reference_dimensions
				canvas_slot.m_translation = glm::vec2(0.0f, 0.0f);
				canvas_slot.m_rotation = 0.0f;

				Ui_widget_vertical_box& vbox = inout_context.create_widget<Ui_widget_vertical_box>("vbox");

				inout_context.add_child(*canvas, vbox, std::move(canvas_slot));

				{
					Ui_widget_image& image = inout_context.create_widget<Ui_widget_image>("test_image");
					image.m_material = ui_mat;
					image.m_image_size = glm::vec2(128.0f, 128.0f);
					inout_context.add_child(vbox, image, Ui_slot_vertical_box().h_align(Ui_h_alignment::left));
				}
				{
					Ui_widget_image& image = inout_context.create_widget<Ui_widget_image>("test_image2");
					image.m_material = ui_mat;
					image.m_image_size = glm::vec2(128.0f, 256.0f);
					inout_context.add_child(vbox, image, Ui_slot_vertical_box().padding(Ui_padding(8.0f)));
				}

				Ui_widget_horizontal_box& vbox2 = inout_context.create_widget<Ui_widget_horizontal_box>("vbox2");

				inout_context.add_child(vbox, vbox2, Ui_slot_vertical_box().h_align(Ui_h_alignment::center));

				{
					Ui_widget_image& image = inout_context.create_widget<Ui_widget_image>("test_image3");
					image.m_material = ui_mat;
					image.m_image_size = glm::vec2(64.0f, 128.0f);
					inout_context.add_child(vbox2, image, Ui_slot_horizontal_box().v_align(Ui_v_alignment::center));
				}
				{
					Ui_widget_image& image = inout_context.create_widget<Ui_widget_image>("test_image4");
					image.m_material = ui_mat;
					image.m_image_size = glm::vec2(128.0f, 64.0f);
					inout_context.add_child(vbox2, image, Ui_slot_horizontal_box().v_align(Ui_v_alignment::center));
				}
 			}
 		);
	}

	void on_tick(Scene_context in_context, float in_time_delta) const override
	{
		in_time_delta;

		const glm::vec3 first_light_pos = glm::vec3(100.0f, 100.0f, -300.0f);

		const glm::vec4 shape_color_green = { 1.0f, 0.0f, 1.0f, 1.0f };
		const glm::vec4 shape_color_cyan = { 0.0f, 1.0f, 1.0f, 1.0f };

		static float cur_val = 0.0f;
		cur_val += in_time_delta;
		const glm::vec3 second_light_pos = glm::vec3(100.0f, (glm::cos(cur_val) * 300.0f), 0.0f);

		//debug_draw::line(in_context, second_light_pos, first_light_pos, shape_color_green);
		//debug_draw::sphere(in_context, second_light_pos, 320.0f, 16, shape_color_cyan);
		//
		//debug_draw::cube(in_context, second_light_pos, glm::vec3(320.0f, 320.0f, 320.0f), glm::quat(glm::vec3(0.0f, 0.0f, cur_val)), shape_color_green);
		//
		//debug_draw::cone(in_context, first_light_pos, glm::normalize(second_light_pos - first_light_pos), 320.0f, glm::radians(45.0f), glm::radians(45.0f), 16, shape_color_cyan);
		//
		//debug_draw::capsule(in_context, first_light_pos, 200.0f, 100.0f, glm::quat(glm::vec3(0.0f, 0.0f, cur_val)), glm::vec4(1.0f, 0.0f, 1.0f, 1.0f));
		//
		for (size_t i = 0; i < 100; i++)
			debug_draw::capsule(in_context, glm::vec3(i * 100.0f, 0.0f, 0.0f), 200.0f, 100.0f, glm::quat(glm::vec3(0.0f, 0.0f, cur_val)), glm::vec4(1.0f, 0.0f, 1.0f, 1.0f));
		//
		//
		//debug_draw::cube(in_context, { 50.0f, 50.0f, -200.0f }, glm::vec3(51, 51, 51), glm::quat(), shape_color_green);

		in_context.process<Processor_flag_read<Component_view>>().for_each_r<Component_view>
		(
			[&in_context](const Component_view& in_view)
			{
				if (auto window = in_view.get_window())
					if (!window->m_is_focused)
						debug_draw::frustum(in_context, in_view.get_owner().find<Component_transform>()->get_matrix(), in_view.calculate_projection_matrix(), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
			}
		);


		Engine_context engine_context = in_context.get_engine_context();
		State_input* input_state = engine_context.get_state<State_input>();

		if (input_state->is_key_pressed(sic::Key::u))
		{
			in_context.process<Processor_flag_read<Player>>().for_each_r<Player>
			(
				[&in_context](const Player&)
				{
					SIC_LOG_W(g_log_game, "test");
				}
			);

			in_context.process<Processor_flag_write<Component_model>>().for_each_w<Component_model>
			(
				[&in_context](Component_model& model)
				{
					in_context.destroy_object(model.get_owner());
				}
			);

			engine_context.get_state<State_assetsystem>()->force_unload_unreferenced_assets();
		}

		if (input_state->is_key_pressed(Key::escape))
		{
			engine_context.shutdown();
		}

		if (input_state->is_key_pressed(Key::r))
		{
			engine_context.get_state_checked<State_renderer>().push_synchronous_renderer_update
			(
				[](Engine_context& inout_context)
				{
					State_assetsystem& assetsystem_state = inout_context.get_state_checked<State_assetsystem>();
					State_renderer_resources& renderer_resources_state = inout_context.get_state_checked<State_renderer_resources>();

					assetsystem_state.for_each<Asset_material>
					(
						[&renderer_resources_state](Asset_material& in_material)
						{
							in_material.m_program.reset();

							std::optional<std::string> vertex_code = Shader_parser::parse_shader(in_material.m_vertex_shader_path.c_str());
							std::optional<std::string> fragment_code = Shader_parser::parse_shader(in_material.m_fragment_shader_path.c_str());

							if (vertex_code.has_value())
								in_material.m_vertex_shader_code = vertex_code.value();

							if (fragment_code.has_value())
								in_material.m_fragment_shader_code = fragment_code.value();

							System_renderer::post_load_material(renderer_resources_state, in_material);
						}
					);
				}
			);
		}

		if (input_state->is_key_down(Key::m))
		{
			in_context.process<Processor_flag_write<Player>>().for_each_w<Player>
			(
				[&in_context, in_time_delta](Player& in_player)
				{
					float& t = in_player.get<Component_player_move>().m_t;
					t += in_time_delta * 1.5f;
					in_player.get<Component_transform>().translate({ 0.0f, glm::cos(t) * 10.0f, 0.0f});

					const glm::vec4 color = { glm::cos(t) * 5.0f, glm::cos(t + 50.0f) * 5.0f, glm::cos(t * 0.1f) * 5.0f, 0.0f };
					auto& dynamic_params = in_player.get<Component_player_move>().m_parameters;

					dynamic_params.set_parameter(in_context, "color_additive", color);
				}
			);
		}
		
		if (input_state->is_key_pressed(Key::a))
		{
			auto processor = in_context.process<Processor_flag_write<Component_player_move>>();

			processor.for_each_w<Component_player_move>
			(
				[&in_context, in_time_delta](Component_player_move& in_player)
				{
					in_player.m_parameters.set_parameter(in_context, "tex_albedo", Asset_ref<Asset_texture>());
				}
			);
			
		}
	}
};



int main()
{
	using namespace sic;

	//sic::g_log_game_verbose.m_enabled = false;
	sic::g_log_asset_verbose.m_enabled = false;
	//sic::g_log_renderer_verbose.m_enabled = false;

	//Ui_widget_canvas canvas;
	//canvas.m_reference_dimensions = { 1920.0f, 1080.0f };
	//
	//Ui_slot_canvas canvas_slot;
	//canvas_slot.m_anchors.set(0.0f);
	//canvas_slot.m_pivot = { 0.5f, 0.5f };
	//canvas_slot.m_size = { 64.0f, 64.0f }; //relative to parents reference_dimensions
	//canvas_slot.m_translation = { 1920.0f / 2.0f, 1080.0f / 2.0f };
	//canvas_slot.m_rotation = 0.0f;
	//
	//Ui_widget_image image;
	//image.m_texture_ref = Asset_ref<Asset_texture>();
	//canvas.add_child(std::move(canvas_slot), std::move(image));
	//
	//glm::vec2 render_size = { 0.0f, 0.0f };
	//glm::vec2 render_translation = { 0.0f, 0.0f };
	//float render_rotation = 0.0f;
	//
	//canvas.m_widgets.back()->calculate_render_transform(render_size, render_translation, render_rotation);	

	{
		std::unique_ptr<Engine> engine_instance = std::make_unique<Engine>();

		engine_instance->add_system<System_file>(Tickstep::async);

		engine_instance->add_system<System_asset>(Tickstep::pre_tick);
		engine_instance->add_system<System_window>(Tickstep::pre_tick);
		engine_instance->add_system<System_input>(Tickstep::pre_tick);
		engine_instance->add_system<System_ui>(Tickstep::pre_tick);

		engine_instance->add_system<System_renderer>(Tickstep::tick);
		engine_instance->add_system<System_model>(Tickstep::tick);

		engine_instance->add_system<System_game>(Tickstep::tick);

		engine_instance->add_system<System_renderer_state_swapper>(Tickstep::post_tick);

		engine_instance->finalize();

		while (!engine_instance->is_shutting_down())
			engine_instance->simulate();
	}

	return 0;
}
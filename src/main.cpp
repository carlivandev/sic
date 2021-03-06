#include "pch.h"

#include "sic/core/engine_context.h"
#include "sic/core/scene_context.h"
#include "sic/core/object.h"
#include "sic/core/leavable_queue.h"
#include "sic/core/logger.h"
#include "sic/core/weak_ref.h"
#include "sic/core/random.h"

#include "sic/system_window.h"
#include "sic/system_input.h"
#include "sic/system_editor_view_controller.h"
#include "sic/system_model.h"
#include "sic/system_file.h"
#include "sic/system_asset.h"
#include "sic/system_renderer.h"
#include "sic/system_renderer_state_swapper.h"
#include "sic/system_ui.h"

#include "sic/system_view.h"
#include "sic/component_transform.h"
#include "sic/file_management.h"
#include "sic/asset_types.h"
#include "sic/debug_draw_helpers.h"
#include "sic/material_dynamic_parameters.h"

#include "sic/editor/asset_import.h"
#include "sic/editor/system_editor.h"

#include "sic/shader_parser.h"
#include <type_traits>
#include <fstream>
#include <filesystem>
#include <any>

using namespace sic;

struct Component_player_move : Component
{
	float m_t = 0.0f;
	Material_dynamic_parameters m_parameters;

	void test(float in, const std::string& in_str)
	{
		m_t = in;
		std::cout << in_str << std::endl;
	}
};

template<>
void sic::rtti::register_type<Component_player_move>(Type_reg&& in_registrator)
{
	in_registrator
		.field("t", &Component_player_move::m_t)
	;
}

struct Player : Object<Player, Component_transform, Component_model, Component_player_move> {};

struct Component_wave : Component
{
	float m_t = 0.0f;
};

struct Object_test_sphere : Object<Object_test_sphere, Component_transform, Component_model, Component_wave> {};

struct System_game : sic::System
{
	void on_created(Engine_context in_context) override
	{
		in_context.register_component_type<Component_transform>("transform");
		in_context.register_component_type<Component_player_move>("Component_player_move");

		Component_player_move test;

		if (const rtti::Typeinfo* typeinfo = in_context.get_typeinfo<Component_player_move>())
		{
			float ok = 24.0f;
			typeinfo->get_field("t")->set(test, ok);
			auto data = typeinfo->serialize_instance(test);
			test.m_t = 100.0f;

			typeinfo->deserialize_instance(Deserialize_stream(data.m_bytes), test);

			auto nice = typeinfo->get_field("t")->get(test);
			if (float* as_float = std::any_cast<float>(&nice))
			{
			}
		}

		in_context.register_object<Player>("Player");

		in_context.register_component_type<Component_wave>("Component_wave");
		in_context.register_object<Object_test_sphere>("Object_model");

		in_context.create_subsystem<System_editor_view_controller>(*this);
	}

	void on_engine_finalized(Engine_context in_context) const override
	{
		in_context.create_scene(nullptr);

		sic::editor::create_editor_window(sic::editor::Processor_editor_window(in_context), "main_window", { 1600, 800 });
		sic::editor::create_editor_window(sic::editor::Processor_editor_window(in_context), "second_window", { 1600, 800 });
	}

	void create_wavy_spheres_test_scene(Scene_context in_context) const
	{
		State_assetsystem& assetsystem_state = in_context.get_engine_context().get_state_checked<State_assetsystem>();
		
		auto create_texture = [&](const std::string& texture_name, const std::string& texture_source, bool in_flip_vertically = false) -> Asset_ref<Asset_texture>
		{
			auto ref = assetsystem_state.create_asset<Asset_texture>(texture_name, "content/textures/sphere_test");
			assetsystem_state.modify_asset<Asset_texture>
			(
				ref,
				[&texture_source, in_flip_vertically](Asset_texture* texture)
				{
					sic::import_texture(*texture, File_management::load_file(texture_source), true, in_flip_vertically);
				}
			);

			return ref;
		};

		auto create_opaque_material = [&](const std::string& material_name, Asset_ref<Asset_texture> in_albedo, Asset_ref<Asset_texture> in_normal, Asset_ref<Asset_texture> in_ao, Asset_ref<Asset_texture> in_r, Asset_ref<Asset_texture> in_m) -> Asset_ref<Asset_material>
		{
			auto ref = assetsystem_state.create_asset<Asset_material>(material_name, "content/materials/sphere_test");
			const std::string vertex_shader = "content/materials/sphere_test/opaque_material_instanced.vert";
			const std::string fragment_shader = "content/materials/sphere_test/opaque_material_instanced.frag";

			assetsystem_state.modify_asset<Asset_material>
			(
				ref,
				[&](Asset_material* material)
				{
					sic::import_material(*material, vertex_shader.c_str(), fragment_shader.c_str());
					material->set_texture_parameter("tex_albedo", in_albedo.as<Asset_texture_base>());
					material->set_texture_parameter("tex_normal", in_normal.as<Asset_texture_base>());
					material->set_texture_parameter("tex_ao", in_ao.as<Asset_texture_base>());
					material->set_texture_parameter("tex_r", in_r.as<Asset_texture_base>());
					material->set_texture_parameter("tex_m", in_m.as<Asset_texture_base>());

					material->m_blend_mode = Material_blend_mode::Opaque;

					Material_data_layout layout;
					layout.add_entry<glm::mat4x4>("MVP");
					layout.add_entry<glm::mat4x4>("model_matrix");
					layout.add_entry<GLuint64>("tex_albedo");
					layout.add_entry<GLuint64>("tex_normal");
					layout.add_entry<GLuint64>("tex_ao");
					layout.add_entry<GLuint64>("tex_r");
					layout.add_entry<GLuint64>("tex_m");

					material->initialize(1024, layout);
					material->set_is_instanced(true);
				}
			);

			return ref;
		};

		auto create_model = [&](const std::string& in_model_name, const std::string& in_model_source, Asset_ref<Asset_material> in_material, const std::string& in_material_slot) -> Asset_ref<Asset_model>
		{
			auto ref = assetsystem_state.create_asset<Asset_model>(in_model_name, "content/models/sphere_test");
			assetsystem_state.modify_asset<Asset_model>
			(
				ref,
				[&in_model_source, &in_material, &in_material_slot](Asset_model* model)
				{
					sic::import_model(*model, File_management::load_file(in_model_source));
					model->set_material(in_material, in_material_slot.c_str());
				}
			);

			return ref;
		};

		auto create_test_object = [&](const Asset_ref<Asset_model>& in_model, const glm::vec3& in_translation, const glm::vec3& in_euler_angles, const glm::vec3& in_scale)
		{
			auto& sphere = in_context.create_object<Object_test_sphere>();
			in_context.get_w<Component_transform>(sphere).set_translation(in_translation);
			in_context.get_w<Component_transform>(sphere).set_rotation(in_euler_angles);
			in_context.get_w<Component_transform>(sphere).set_scale(in_scale);
			in_context.get_w<Component_model>(sphere).set_model(Processor_model(in_context), in_model);
		};

		auto sphere_albedo = create_texture("sphere_albedo", "content/textures/sphere/t_sphere_a.png");
		auto sphere_normal = create_texture("sphere_normal", "content/textures/sphere/t_sphere_n.png");
		auto sphere_ao = create_texture("sphere_ao", "content/textures/sphere/t_sphere_ao.png");
		auto sphere_r = create_texture("sphere_r", "content/textures/sphere/t_sphere_r.png");
		auto sphere_m = create_texture("sphere_m", "content/textures/sphere/t_sphere_m.png");

		auto sphere_material = create_opaque_material("sphere_material", sphere_albedo, sphere_normal, sphere_ao, sphere_r, sphere_m);

		auto sphere_model = create_model("sphere_model", "content/models/sm_sphere.fbx", sphere_material, "lambert2");

		
		
		for (size_t x = 0; x < 10; x++)
		{
			for (size_t z = 0; z < 10; z++)
			{
				const float dist_between_spheres = 150.0f;
				
				create_test_object(sphere_model, { x * dist_between_spheres, 0.0f, z * dist_between_spheres }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f });
			}
		}
		
	}

	void on_begin_simulation(Scene_context in_context) const override
	{
		State_window* window_state = in_context.get_engine_context().get_state<State_window>();
		auto main_window = window_state->find_window("main_window");
		auto secondary_window = window_state->find_window("second_window");

		Component_view& main_view = in_context.get_w<Component_view>(in_context.create_object<Object_view>());
		main_view.set_window(main_window);
		main_view.set_near_and_far_plane(10.0f, 50000.0f);

		//main_view.set_clear_color({ 0.0f, 0.0f, 0.1f, 1.0f });
		//main_view.set_viewport_dimensions(main_window->get_dimensions());

		Component_view& secondary_view = in_context.get_w<Component_view>(in_context.create_object<Object_view>());
		secondary_view.set_window(secondary_window);
		secondary_view.set_viewport_size({ 1.0f, 1.0f });
		secondary_view.set_viewport_offset({ 0.0f, 0.0f });
		
		in_context.find_w<Component_transform>(secondary_view.get_owner())->set_translation({ -400.0f, 0.0f, 0.0f });
		in_context.find_w<Component_transform>(secondary_view.get_owner())->look_at(glm::normalize(glm::vec3(50.0f, 50.0f, -200.0f) - glm::vec3{ -400.0f, 0.0f, 0.0f }), Transform::up);

		//secondary_view.set_clear_color({ 0.0f, 0.0f, 0.1f, 1.0f });
		//secondary_view.set_viewport_dimensions(secondary_window->get_dimensions());

		in_context.get_w<Component_editor_view_controller>(in_context.create_object<Object_editor_view_controller>()).m_view_to_control = &main_view;

		State_assetsystem* assetsystem_state = in_context.get_engine_context().get_state<State_assetsystem>();

		auto main_view_rt_ref = assetsystem_state->create_asset<Asset_render_target>("main_view_render_target", "content/render_targets");
		assetsystem_state->modify_asset<Asset_render_target>
		(
			main_view_rt_ref,
			[](Asset_render_target* rt)
			{
				rt->initialize(Texture_format::rgb, { 1600, 800 }, true);
			}
		);

		main_view.set_render_target(main_view_rt_ref);

		auto second_view_rt_ref = assetsystem_state->create_asset<Asset_render_target>("second_view_render_target", "content/render_targets");
		assetsystem_state->modify_asset<Asset_render_target>
		(
			second_view_rt_ref,
			[](Asset_render_target* rt)
			{
				rt->initialize(Texture_format::rgb, { 1600, 800 }, true);
			}
		);

		secondary_view.set_render_target(second_view_rt_ref);

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
					sic::import_material(*material, vertex_shader.c_str(), fragment_shader.c_str());
					if (in_blend_mode == Material_blend_mode::Opaque)
					{
						material->set_texture_parameter("tex_albedo", Asset_ref<Asset_texture_base>());
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

			assetsystem_state->save_asset<Asset_material>(ref);

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
					material->set_texture_parameter("tex_albedo", in_texture.as<Asset_texture_base>());
				}
			);

			assetsystem_state->save_asset(ref);

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
					sic::import_material(*material, vertex_shader.c_str(), fragment_shader.c_str());
					if (in_blend_mode == Material_blend_mode::Opaque)
						material->set_texture_parameter("tex_albedo", in_texture.as<Asset_texture_base>());

					material->m_blend_mode = in_blend_mode;

					Material_data_layout layout;
					layout.add_entry<glm::mat4x4>("MVP");
					layout.add_entry<glm::mat4x4>("model_matrix");
					layout.add_entry<GLuint64>("tex_albedo");

					material->initialize(1024, layout);
					material->set_is_instanced(in_instanced);
				}
			);

			assetsystem_state->save_asset(ref);

			return ref;
		};

#define SIC_WAVYSPHERES_TEST

#ifdef SIC_WAVYSPHERES_TEST

		create_wavy_spheres_test_scene(in_context);
#else
#if 1

		auto tex_body =		(create_test_texture("tex_crysis_body", "content/textures/crysis/body_dif.png"));
		auto tex_glass =	(create_test_texture("tex_crysis_glass", "content/textures/crysis/glass_dif.png"));
		auto tex_arm =		(create_test_texture("tex_crysis_arm", "content/textures/crysis/arm_dif.png"));
		auto tex_hand =		(create_test_texture("tex_crysis_hand", "content/textures/crysis/hand_dif.png"));
		auto tex_helmet =	(create_test_texture("tex_crysis_helmet", "content/textures/crysis/helmet_dif.png"));
		auto tex_leg =		(create_test_texture("tex_crysis_leg", "content/textures/crysis/leg_dif.png"));
// 
// 		{
// 			Asset_ref<Asset_texture_base> test_ref = tex_glass.as<Asset_texture_base>();
// 			test_ref = Asset_ref<Asset_texture_base>();
// 			test_ref = tex_glass.as<Asset_texture_base>();
// 			test_ref = Asset_ref<Asset_texture_base>();
// 			test_ref = std::move(tex_glass.as<Asset_texture_base>());
// 			test_ref = Asset_ref<Asset_texture_base>();
// 		}

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
				sic::import_model(*model, File_management::load_file("content/models/crysis.fbx"));
				model->set_material(mat_body, "Body");
				model->set_material(mat_glass, "Glass");
				model->set_material(mat_arm, "Arm");
				model->set_material(mat_hand, "Hand");
				model->set_material(mat_helmet, "Helmet");
				model->set_material(mat_leg, "Leg");
			}
		);

		assetsystem_state->save_asset(model_ref);
#else
		Asset_ref<Asset_model> model_ref = assetsystem_state->find_asset<Asset_model>(xg::Guid("46aae3ac-64f8-4db1-82a8-691378774aee"));
#endif

		Processor_model render_scene_processor(in_context);

		{
			Player& new_player = in_context.create_object<Player>();
			Component_model& player_model = in_context.get_w<Component_model>(new_player);
			player_model.set_model(render_scene_processor, model_ref);

			in_context.get_w<Component_transform>(new_player).set_translation({ 50.0f, 50.0f, -200.0f });
			in_context.get_w<Component_transform>(new_player).set_scale({ 10.0f, 10.0f, 10.0f });

			in_context.get_w<Component_player_move>(new_player).m_parameters.bind(render_scene_processor, player_model);
		}

		{
			Player& new_player = in_context.create_object<Player>();
			Component_model& player_model = in_context.get_w<Component_model>(new_player);
			player_model.set_model(render_scene_processor, model_ref);

			in_context.get_w<Component_transform>(new_player).set_translation({ 200.0f, 0.0f, 0.0f });
			in_context.get_w<Component_transform>(new_player).look_at(glm::normalize(glm::vec3(50.0f, 50.0f, -200.0f) - glm::vec3(200.0f, 0.0f, 0.0f)), sic::Transform::up);
			in_context.get_w<Component_transform>(new_player).set_scale({ 10.0f, 20.0f, 10.0f });

			in_context.get_w<Component_player_move>(new_player).m_parameters.bind(render_scene_processor, player_model, "Leg");
			in_context.get_w<Component_player_move>(new_player).m_parameters.set_parameter(render_scene_processor, "tex_albedo", tex_glass.as<Asset_texture_base>());
		}

		for (size_t x = 0; x < 20; x++)
		{
			for (size_t y = 0; y < 40; y++)
			{
				Player& new_player = in_context.create_object<Player>();
				Component_model& player_model = in_context.get_w<Component_model>(new_player);
				player_model.set_model(render_scene_processor, model_ref);

				in_context.get_w<Component_transform>(new_player).set_translation({ x * 80.0f, y * 80.0f, 0.0f });
				in_context.get_w<Component_transform>(new_player).look_at(glm::normalize(glm::vec3(50.0f, 50.0f, -200.0f) - glm::vec3(200.0f, 0.0f, 0.0f)), sic::Transform::up);
				in_context.get_w<Component_transform>(new_player).set_scale({ 10.0f, 10.0f, 10.0f });
				in_context.get_w<Component_player_move>(new_player).m_t = (x + y * 0.5f) * 10.0f;
				in_context.get_w<Component_player_move>(new_player).m_parameters.bind(render_scene_processor, player_model, "Hand");

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

#endif

		SIC_LOG(g_log_game, "ok");
		SIC_LOG_W(g_log_game, "hmmm!");
		SIC_LOG_E(g_log_game, "oh no!");
		SIC_LOG_V(g_log_game, "it works!");

		auto test_font_ref = assetsystem_state->create_asset<Asset_font>("test_font", "content/fonts");
		{
			assetsystem_state->modify_asset<Asset_font>
				(
					test_font_ref,
					[&](Asset_font* font)
					{
						Asset_font::Import_configuration conf;
						sic::import_font(*font, "content/fonts/arial.ttf", conf);
					}
			);

			assetsystem_state->save_asset(test_font_ref);
		}

		auto create_test_ui_text_material = [&](const std::string& material_name, Asset_ref<Asset_font> in_font, Material_blend_mode in_blend_mode, bool in_instanced) -> Asset_ref<Asset_material>
		{
			auto ref = assetsystem_state->create_asset<Asset_material>(material_name, "content/materials/ui");
			std::string vertex_shader = "content/engine/materials/default_ui.vert";
			std::string fragment_shader = "content/engine/materials/default_ui_text.frag";

			if (in_instanced)
			{
				vertex_shader = "content/engine/materials/default_ui_instanced.vert";
				fragment_shader = "content/engine/materials/default_ui_text_instanced.frag";
			}

			assetsystem_state->modify_asset<Asset_material>
			(
				ref,
				[&](Asset_material* material)
				{
					sic::import_material(*material, vertex_shader.c_str(), fragment_shader.c_str());

					material->set_texture_parameter("msdf", in_font.as<Asset_texture_base>());
					material->set_vec4_parameter("offset_and_size", { 0.0f, 0.0f, 0.0f, 0.0f });
					material->set_vec4_parameter("lefttop_rightbottom_packed", { 0.0f, 0.0f, 0.0f, 0.0f });
					material->set_vec4_parameter("fg_color", { 1.0f, 1.0f, 1.0f, 1.0f });
					material->set_vec4_parameter("bg_color", { 0.0f, 0.0f, 0.0f, 0.0f });

					material->m_blend_mode = in_blend_mode;

					Material_data_layout layout;
					layout.add_entry<glm::vec4>("lefttop_rightbottom_packed");
					layout.add_entry<GLuint64>("msdf");
					layout.add_entry<glm::vec4>("offset_and_size");
					layout.add_entry<glm::vec4>("fg_color");
					layout.add_entry<glm::vec4>("bg_color");

					material->initialize(1024, layout);
					material->set_is_instanced(in_instanced);
				}
			);

			assetsystem_state->save_asset(ref);

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

		auto tex_ui_test = create_test_texture("tex_ui_test", "content/textures/ui_test_image.png", true);
		auto ui_mat = create_test_ui_material("ui_material_test", tex_ui_test.as<Asset_texture_base>(), Material_blend_mode::Translucent, true);
		
		auto game_view_ui_mat = create_test_ui_material("ui_material_game_view", main_view_rt_ref.as<Asset_texture_base>(), Material_blend_mode::Translucent, true);
		auto game_secondary_view_ui_mat = create_test_ui_material("ui_material_second_game_view", second_view_rt_ref.as<Asset_texture_base>(), Material_blend_mode::Translucent, true);
		auto font_material = create_test_ui_text_material("ui_material_game_view", test_font_ref, Material_blend_mode::Translucent, true);

		Processor_window(in_context).update_state_deferred<State_ui>
		(
			[=](State_ui& ui_state)
			{
				{
					Ui_widget_canvas* canvas = ui_state.find<Ui_widget_canvas>("main_window_content_area");
					assert(canvas);

					canvas->add_child
					(
						Ui_slot_canvas()
						.anchors(Ui_anchors(0.0f, 0.0f, 1.0f, 1.0f))
						.pivot({ 0.5f, 0.5f })
						.size(canvas->m_reference_dimensions) //relative to parents reference_dimensions
						.translation(glm::vec2(0.0f, 0.0f))
						.rotation(0.0f),

						ui_state.create<Ui_widget_image>("game_view_1")
						.image_size({ 1920.0f, 1080.0f })
						.material(game_view_ui_mat)
					);
				}

				{
					Ui_widget_canvas* canvas = ui_state.find<Ui_widget_canvas>("second_window_content_area");
					assert(canvas);

					canvas->add_child
					(
						Ui_slot_canvas()
						.anchors(Ui_anchors(1.0f, 1.0f))
						.pivot({ 1.0f, 1.0f })
						.size(canvas->m_reference_dimensions * 0.5f) //relative to parents reference_dimensions
						.translation(glm::vec2(0.0f, 0.0f))
						.rotation(0.0f),

						ui_state.create<Ui_widget_image>("game_view_2")
						.image_size({ 1600.0f, 800.0f })
						.material(game_secondary_view_ui_mat)
					);

					canvas->add_child
					(
						Ui_slot_canvas()
						.anchors(Ui_anchors(1.0f, 1.0f))
						.pivot({ 1.0f, 1.0f })
						.size(canvas->m_reference_dimensions * 0.5f) //relative to parents reference_dimensions
						.translation(glm::vec2(0.0f, 0.0f))
						.rotation(0.0f),

						//inout_context.create<Ui_widget_image>("game_view_2")
						//.image_size({ 1600.0f, 800.0f })
						//.material(game_secondary_view_ui_mat)
						ui_state.create<Ui_widget_text>("test_text")
						.text("it little Ghello FRIEND g Tj q @Aa. , - AV\nok multi-line NEAT i LIKE IT awf awf awf awf awf awf awf awf awf awf awf awf")
						.font(test_font_ref)
						.material(font_material)
						.px(32.0f)
						.align(Ui_h_alignment::right)
						.autowrap(true)
					);
					canvas->add_child
					(
						Ui_slot_canvas()
						.anchors(Ui_anchors(1.0f, 1.0f))
						.pivot({ 1.0f, 1.0f })
						.size(canvas->m_reference_dimensions * 0.25f) //relative to parents reference_dimensions
						.translation(glm::vec2(0.0f, 0.0f))
						.rotation(0.0f),

						//inout_context.create<Ui_widget_image>("game_view_2")
						//.image_size({ 1600.0f, 800.0f })
						//.material(game_secondary_view_ui_mat)
						ui_state.create<Ui_widget_text>("test_text_2")
						.text("i am little text hello :)")
						.font(test_font_ref)
						.material(font_material)
						.px(16.0f)
					);

				}
			}
		);
	}

	static void draw_frustums(Processor<Processor_flag_read<Component_view>, Processor_flag_read_single<Component_transform>, Processor_debug_draw> in_processor)
	{
		in_processor.for_each_r<Component_view>
		(
			[&in_processor](const Component_view& in_view)
			{
				if (auto window = in_view.get_window())
					if (!window->m_is_focused)
						debug_draw::frustum(in_processor, in_processor.find_r<Component_transform>(in_view.get_owner())->get_matrix(), in_view.calculate_projection_matrix(), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
			}
		);
	}

	static void draw_capsules(Processor<Processor_debug_draw> in_processor)
	{
		static float cur_val = 0.0f;
		cur_val += in_processor.get_time_delta();

		for (size_t i = 0; i < 100; i++)
			debug_draw::capsule(in_processor, glm::vec3(i * 100.0f, 0.0f, 0.0f), 200.0f, 100.0f, glm::quat(glm::vec3(0.0f, 0.0f, cur_val)), glm::vec4(1.0f, 0.0f, 1.0f, 1.0f));
	}

	static void update_players(Processor<Processor_flag_write<Player>, Processor_flag_write<Component_player_move>, Processor_flag_write_single<Component_transform>, Processor_flag_read<State_input>, Processor_render_scene_update> in_processor)
	{
		const auto& input_state = in_processor.get_state_checked_r<State_input>();

		if (input_state.is_key_down(Key::m))
		{
			in_processor.for_each_w<Player>
			(
				[in_processor](Player& in_player) mutable
				{
					float& t = in_processor.get_w<Component_player_move>(in_player).m_t;
					t += in_processor.get_time_delta() * 1.5f;
					in_processor.get_w<Component_transform>(in_player).translate({ 0.0f, glm::cos(t) * 10.0f, 0.0f});

					const glm::vec4 color = { glm::cos(t) * 5.0f, glm::cos(t + 50.0f) * 5.0f, glm::cos(t * 0.1f) * 5.0f, 0.0f };
					auto& dynamic_params = in_processor.get_w<Component_player_move>(in_player).m_parameters;

					dynamic_params.set_parameter(in_processor, "color_additive", color);
				}
			);
		}
		
		if (input_state.is_key_pressed(Key::a))
		{
			in_processor.for_each_w<Component_player_move>
			(
				[in_processor](Component_player_move& in_player)
				{
					in_player.m_parameters.set_parameter(in_processor, "tex_albedo", Asset_ref<Asset_texture_base>());
				}
			);
		}
	}

	static void update_waves(Processor<Processor_flag_write<Object_test_sphere>, Processor_flag_write<Component_wave>, Processor_flag_write_single<Component_transform>, Processor_render_scene_update, Processor_debug_draw> in_processor)
	{
		//SIC_LOG_W(g_log_game, "starts on thread: {0}", this_thread().get_name());
		in_processor.for_each_w<Object_test_sphere>
		(
			[&in_processor](Object_test_sphere& in_sphere)
			{
				const float wave_speed = 0.8f;
				const float wave_height = 500.0f;

				float& t = in_processor.get_w<Component_wave>(in_sphere).m_t;
				t += in_processor.get_time_delta() * wave_speed;

				Component_transform& transform = in_processor.get_w<Component_transform>(in_sphere);

				const float positional_offset = (transform.get_translation().x + transform.get_translation().z) * 0.001f;
				const float new_y = glm::cos(t + positional_offset) * wave_height;

				transform.set_translation({ transform.get_translation().x, new_y, transform.get_translation().z });
			}
		);

		//SIC_LOG_W(g_log_game, "finishes on thread: {0}", this_thread().get_name());
	}

	static void update_debug_window_creation(Processor<Processor_flag_read<State_input>, sic::editor::Processor_editor_window> in_processor)
	{
		static i32 window_name_ticker = 0;
		
		if (in_processor.get_state_checked_r<State_input>().is_key_pressed(Key::v))
			sic::editor::create_editor_window(in_processor, "debug_window_" + std::to_string(window_name_ticker++), { 128.0f, 128.0f });
	}

	void on_tick(Scene_context in_context, float in_time_delta) const override
	{
		in_time_delta;

		in_context.schedule(update_waves);
		in_context.schedule(update_debug_window_creation);

// 		const glm::vec3 first_light_pos = glm::vec3(100.0f, 100.0f, -300.0f);
// 
// 		const glm::vec4 shape_color_green = { 1.0f, 0.0f, 1.0f, 1.0f };
// 		const glm::vec4 shape_color_cyan = { 0.0f, 1.0f, 1.0f, 1.0f };
// 
// 		static float cur_val = 0.0f;
// 		cur_val += in_time_delta;
// 		const glm::vec3 second_light_pos = glm::vec3(100.0f, (glm::cos(cur_val) * 300.0f), 0.0f);

		//debug_draw::line(in_context, second_light_pos, first_light_pos, shape_color_green);
		//debug_draw::sphere(in_context, second_light_pos, 320.0f, 16, shape_color_cyan);
		//
		//debug_draw::cube(in_context, second_light_pos, glm::vec3(320.0f, 320.0f, 320.0f), glm::quat(glm::vec3(0.0f, 0.0f, cur_val)), shape_color_green);
		//
		//debug_draw::cone(in_context, first_light_pos, glm::normalize(second_light_pos - first_light_pos), 320.0f, glm::radians(45.0f), glm::radians(45.0f), 16, shape_color_cyan);
		//
		//debug_draw::capsule(in_context, first_light_pos, 200.0f, 100.0f, glm::quat(glm::vec3(0.0f, 0.0f, cur_val)), glm::vec4(1.0f, 0.0f, 1.0f, 1.0f));
		//

		//
		//
		//debug_draw::cube(in_context, { 50.0f, 50.0f, -200.0f }, glm::vec3(51, 51, 51), glm::quat(), shape_color_green);

		

		//auto frustum_job_id = in_context.schedule(&draw_frustums);
		//in_context.schedule(&draw_capsules, Schedule_data().job_dependency(frustum_job_id));

		Engine_context engine_context = in_context.get_engine_context();
		State_input* input_state = engine_context.get_state<State_input>();

		if (input_state->is_key_pressed(sic::Key::u))
		{
			in_context.for_each_r<Player>
			(
				[&in_context](const Player&)
				{
					SIC_LOG_W(g_log_game, "test");
				}
			);

			in_context.for_each_w<Component_model>
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

		in_context.schedule(update_players);
	}
};

int main()
{
	//sic::g_log_game_verbose.m_enabled = false;
	sic::g_log_asset_verbose.m_enabled = false;
	//sic::g_log_renderer_verbose.m_enabled = false;
	sic::g_log_font_verbose.m_enabled = true;
	//sic::g_log_ui_verbose.m_enabled = true;
	{
		std::unique_ptr<Engine> engine_instance = std::make_unique<Engine>();


		engine_instance->add_system<System_asset>(Tickstep::pre_tick);
		engine_instance->add_system<System_window>(Tickstep::pre_tick);
		engine_instance->add_system<System_input>(Tickstep::pre_tick);
		engine_instance->add_system<System_ui>(Tickstep::pre_tick);
		engine_instance->add_system<editor::System_editor>(Tickstep::pre_tick);

		engine_instance->add_system<System_renderer>(Tickstep::tick);
		engine_instance->add_system<System_model>(Tickstep::tick);

		engine_instance->add_system<System_game>(Tickstep::tick);

		engine_instance->add_system<System_renderer_state_swapper>(Tickstep::post_tick);
		engine_instance->add_system<System_file>(Tickstep::post_tick);

		engine_instance->finalize();

		while (!engine_instance->is_shutting_down())
			engine_instance->simulate();
	}

	return 0;
}
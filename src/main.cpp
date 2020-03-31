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

#include "sic/object.h"
#include "sic/system_view.h"
#include "sic/component_transform.h"
#include "sic/file_management.h"
#include "sic/asset_types.h"
#include "sic/leavable_queue.h"
#include "sic/logger.h"
#include "sic/debug_draw_helpers.h"

using namespace sic;

struct System_game : public sic::System
{
	struct player : Object<player, Component_transform, Component_model> {};

	void on_created(Engine_context in_context) override
	{
		in_context.register_component_type<Component_transform>("transform");
		in_context.register_object<player>("player");

		in_context.create_subsystem<System_input>(*this);
		in_context.create_subsystem<System_editor_view_controller>(*this);
		in_context.create_subsystem<System_model>(*this);
	}

	void on_begin_simulation(Level_context in_context) const override
	{
		Object_window& main_window = in_context.create_object<Object_window>();
		in_context.get_engine_context().get_state<State_window>()->m_main_window = &main_window;

		Object_window& secondary_window = in_context.create_object<Object_window>();

		Component_view& main_view = in_context.create_object<Object_view>().get<Component_view>();
		main_view.set_clear_color({ 0.0f, 0.0f, 0.1f, 1.0f });
		main_view.set_viewport_size({ 0.5f, 0.5f });
		main_view.set_viewport_dimensions(main_window.get<Component_window>().m_dimensions);
		main_view.set_window(&main_window);
		main_view.set_near_and_far_plane(0.1f, 1000.0f);

		Component_view& secondary_view = in_context.create_object<Object_view>().get<Component_view>();
		secondary_view.set_clear_color({ 0.0f, 0.0f, 0.1f, 1.0f });
		secondary_view.set_window(&secondary_window);
		secondary_view.set_viewport_size({ 0.5f, 0.5f });
		secondary_view.set_viewport_offset({ 0.5f, 0.5f });
		secondary_view.get_owner().find<Component_transform>()->set_translation({ -40.0f, 0.0f, 0.0f });
		secondary_view.get_owner().find<Component_transform>()->look_at(glm::normalize(glm::vec3(5.0f, 5.0f, -20.0f) - glm::vec3{ -40.0f, 0.0f, 0.0f }), Transform::up);
		secondary_view.set_viewport_dimensions(main_window.get<Component_window>().m_dimensions);

		in_context.create_object<Object_editor_view_controller>().get<Component_editor_view_controller>().m_view_to_control = &main_view;

		State_assetsystem* assetsystem_state = in_context.get_engine_context().get_state<State_assetsystem>();

		auto create_test_texture = [&](const std::string& texture_name, const std::string& texture_source, Texture_format format) -> Asset_ref<Asset_texture>
		{
			auto ref = assetsystem_state->create_asset<Asset_texture>(texture_name, "content/textures/crysis");
			ref.get()->import(File_management::load_file(texture_source), format, true);
			assetsystem_state->save_asset<Asset_texture>(*ref.get_header());

			return ref;
		};

		auto create_test_material = [&](const std::string& material_name, Asset_ref<Asset_texture> in_texture) -> Asset_ref<Asset_material>
		{
			auto ref = assetsystem_state->create_asset<Asset_material>(material_name, "content/materials/crysis");
			std::string vertex_shader = "content/materials/material - Copy.vert";
			std::string fragment_shader = "content/materials/material - Copy.frag";

			ref.get()->import(vertex_shader, File_management::load_file(vertex_shader), fragment_shader, File_management::load_file(fragment_shader));
			ref.get()->m_texture_parameters.push_back({ "tex_albedo", in_texture });
			assetsystem_state->save_asset<Asset_material>(*ref.get_header());

			return ref;
		};

		auto tex_body =		(create_test_texture("tex_crysis_body", "content/textures/crysis/body_dif.png", Texture_format::rgb_a));
		auto tex_glass =	(create_test_texture("tex_crysis_glass", "content/textures/crysis/glass_dif.png", Texture_format::rgb));
		auto tex_arm =		(create_test_texture("tex_crysis_arm", "content/textures/crysis/arm_dif.png", Texture_format::rgb_a));
		auto tex_hand =		(create_test_texture("tex_crysis_hand", "content/textures/crysis/hand_dif.png", Texture_format::rgb_a));
		auto tex_helmet =	(create_test_texture("tex_crysis_helmet", "content/textures/crysis/helmet_dif.png", Texture_format::rgb_a));
		auto tex_leg =		(create_test_texture("tex_crysis_leg", "content/textures/crysis/leg_dif.png", Texture_format::rgb_a));

		auto mat_body =		(create_test_material("mat_crysis_body", tex_body));
		auto mat_glass =	(create_test_material("mat_crysis_glass", tex_glass));
		auto mat_arm =		(create_test_material("mat_crysis_arm", tex_arm));
		auto mat_hand =		(create_test_material("mat_crysis_hand", tex_hand));
		auto mat_helmet =	(create_test_material("mat_crysis_helmet", tex_helmet));
		auto mat_leg =		(create_test_material("mat_crysis_leg", tex_leg));

		Asset_ref<Asset_model> model_ref = assetsystem_state->create_asset<Asset_model>("model_crysis", "content/models");
		
		model_ref.get()->import(File_management::load_file("content/models/crysis.fbx"));
		model_ref.get()->set_material(mat_body, "Body");
		model_ref.get()->set_material(mat_glass, "Glass");
		model_ref.get()->set_material(mat_arm, "Arm");
		model_ref.get()->set_material(mat_hand, "Hand");
		model_ref.get()->set_material(mat_helmet, "Helmet");
		model_ref.get()->set_material(mat_leg, "Leg");

		assetsystem_state->save_asset<Asset_model>(*model_ref.get_header());

		{
			player& new_player = in_context.create_object<player>();
			Component_model& player_model = new_player.get<Component_model>();
			player_model.set_model(model_ref);

			new_player.get<Component_transform>().set_translation({ 5.0f, 5.0f, -20.0f });
		}

		{
			player& new_player = in_context.create_object<player>();
			Component_model& player_model = new_player.get<Component_model>();
			player_model.set_model(model_ref);

			new_player.get<Component_transform>().set_translation({ 20.0f, 0.0f, 0.0f });
			new_player.get<Component_transform>().look_at(glm::normalize(glm::vec3(5.0f, 5.0f, -20.0f) - glm::vec3(20.0f, 0.0f, 0.0f)), sic::Transform::up);
			new_player.get<Component_transform>().set_scale({ 1.0f, 2.0f, 1.0f });
		}

		in_context.get_engine_context().get_state<State_filesystem>()->request_load
		(
			File_load_request("content/models/crysis.fbx",
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
	}

	void on_tick(Level_context in_context, float in_time_delta) const override
	{
		in_time_delta;

		static bool do_once = false;

		if (!do_once)
		{
			do_once = true;
			debug_draw::sphere(in_context, glm::vec3(-40.0f, 0.0f, 0.0f), 32.0f, 16, glm::vec4(0.0f, 1.0f, 1.0f, 1.0f), 5.0f);
			debug_draw::line(in_context, { -40.0f, 0.0f, 0.0f }, { -20.0f, 0.0f, 0.0f }, { 0.5f, 0.0f, 1.0f, 1.0f }, 10.0f);
		}

		Engine_context engine_context = in_context.get_engine_context();
		State_input* input_state = engine_context.get_state<State_input>();

		if (input_state->is_key_pressed(sic::Key::u))
		{
			in_context.for_each<player>
			(
				[&in_context](player&)
				{
					SIC_LOG_W(g_log_game, "test");
				}
			);

			in_context.for_each<Component_model>
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
			in_context.get_engine_context().shutdown();
		}
	}
};

int main()
{
	using namespace sic;

	//sic::g_log_game_verbose.m_enabled = false;
	//sic::g_log_asset_verbose.m_enabled = false;
	//sic::g_log_renderer_verbose.m_enabled = false;

	{
		std::unique_ptr<Engine> engine_instance = std::make_unique<Engine>();

		engine_instance->add_system<System_file>(Tickstep::async);

		engine_instance->add_system<System_asset>(Tickstep::pre_tick);
		engine_instance->add_system<System_window>(Tickstep::pre_tick);

		engine_instance->add_system<System_renderer>(Tickstep::tick);
		engine_instance->add_system<System_game>(Tickstep::tick);

		engine_instance->add_system<System_renderer_state_swapper>(Tickstep::post_tick);

		engine_instance->initialize();

		while (!engine_instance->is_shutting_down())
			engine_instance->simulate();
	}

	return 0;
}
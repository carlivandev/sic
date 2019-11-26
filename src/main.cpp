#include "pch.h"

#include "impuls/world_context.h"

#include "impuls/system_window.h"
#include "impuls/system_input.h"
#include "impuls/system_editor_view_controller.h"
#include "impuls/system_model.h"
#include "impuls/system_file.h"
#include "impuls/system_asset.h"
#include "impuls/system_renderer.h"
#include "impuls/system_renderer_state_swapper.h"

#include "impuls/view.h"
#include "impuls/component_transform.h"
#include "impuls/file_management.h"
#include "impuls/asset_types.h"
#include "impuls/leavable_queue.h"
#include "impuls/logger.h"

using namespace impuls;

struct system_game : public impuls::i_system
{
	struct player : i_object<player, component_transform, component_model> {};

	void on_created(world_context&& in_context) const
	{
		in_context.create_subsystem<system_input>();
		in_context.create_subsystem<system_editor_view_controller>();
		in_context.create_subsystem<system_model>();

		in_context.register_component_type<component_transform>("transform");
		in_context.register_object<player>("player");

		object_window& main_window = in_context.create_object<object_window>();
		in_context.get_state<state_main_window>()->m_window = &main_window;

		component_view& main_view = in_context.create_object<object_view>().get<component_view>();

		main_view.m_window_render_on = &main_window;
		in_context.create_object<object_editor_view_controller>().get<component_editor_view_controller>().m_view_to_control = &main_view;

		state_assetsystem* assetsystem_state = in_context.get_state<state_assetsystem>();

		auto create_test_texture = [&](const std::string& texture_name, const std::string& texture_source, e_texture_format format) -> asset_ref<asset_texture>
		{
			auto ref = assetsystem_state->create_asset<asset_texture>(texture_name, "content/textures/crysis");
			ref.get()->import(file_management::load_file(texture_source), format, true);
			assetsystem_state->save_asset<asset_texture>(*ref.get_header());

			return ref;
		};

		auto create_test_material = [&](const std::string& material_name, asset_ref<asset_texture> in_texture) -> asset_ref<asset_material>
		{
			auto ref = assetsystem_state->create_asset<asset_material>(material_name, "content/materials/crysis");
			ref.get()->m_vertex_shader = "content/materials/material.vs";
			ref.get()->m_fragment_shader = "content/materials/material.fs";
			ref.get()->m_texture_parameters.push_back({ "tex_albedo", in_texture });
			assetsystem_state->save_asset<asset_material>(*ref.get_header());

			return ref;
		};

		auto tex_body =		(create_test_texture("tex_crysis_body", "content/textures/crysis/body_dif.png", e_texture_format::rgb_a));
		auto tex_glass =		(create_test_texture("tex_crysis_glass", "content/textures/crysis/glass_dif.png", e_texture_format::rgb));
		auto tex_arm =		(create_test_texture("tex_crysis_arm", "content/textures/crysis/arm_dif.png", e_texture_format::rgb_a));
		auto tex_hand =		(create_test_texture("tex_crysis_hand", "content/textures/crysis/hand_dif.png", e_texture_format::rgb_a));
		auto tex_helmet =		(create_test_texture("tex_crysis_helmet", "content/textures/crysis/helmet_dif.png", e_texture_format::rgb_a));
		auto tex_leg =		(create_test_texture("tex_crysis_leg", "content/textures/crysis/leg_dif.png", e_texture_format::rgb_a));

		auto mat_body =		(create_test_material("mat_crysis_body", tex_body));
		auto mat_glass =		(create_test_material("mat_crysis_glass", tex_glass));
		auto mat_arm =		(create_test_material("mat_crysis_arm", tex_arm));
		auto mat_hand =		(create_test_material("mat_crysis_hand", tex_hand));
		auto mat_helmet =		(create_test_material("mat_crysis_helmet", tex_helmet));
		auto mat_leg =		(create_test_material("mat_crysis_leg", tex_leg));

		asset_ref<asset_model> model_ref = assetsystem_state->create_asset<asset_model>("model_crysis", "content/models");
		
		model_ref.get()->import(file_management::load_file("content/models/crysis.fbx"));
		model_ref.get()->set_material(mat_body, "Body");
		model_ref.get()->set_material(mat_glass, "Glass");
		model_ref.get()->set_material(mat_arm, "Arm");
		model_ref.get()->set_material(mat_hand, "Hand");
		model_ref.get()->set_material(mat_helmet, "Helmet");
		model_ref.get()->set_material(mat_leg, "Leg");

		assetsystem_state->save_asset<asset_model>(*model_ref.get_header());

		{
			player& new_player = in_context.create_object<player>();
			component_model& player_model = new_player.get<component_model>();
			player_model.set_model(model_ref);

			new_player.get<component_transform>().set_position({ 0.0f, 0.0f, 0.0f });
		}

		{
			player& new_player = in_context.create_object<player>();
			component_model& player_model = new_player.get<component_model>();
			player_model.set_model(model_ref);

			new_player.get<component_transform>().set_position({ 20.0f, 0.0f, 0.0f });
		}

		in_context.get_state<state_filesystem>()->request_load
		(
			file_load_request("content/models/crysis.fbx",
				[](std::string && in_data)
				{
					in_data;
					IMPULS_LOG(g_log_game, "Loaded!");
				}
			)
		);

		IMPULS_LOG(g_log_game, "ok");
		IMPULS_LOG_W(g_log_game, "hmmm!");
		IMPULS_LOG_E(g_log_game, "oh no!");
		IMPULS_LOG_V(g_log_game, "it works!");
	}

	void on_tick(world_context&& in_context, float in_time_delta) const override
	{
		in_time_delta;

		if (in_context.get_state<state_input>()->is_key_pressed(impuls::e_key::u))
		{
			for (component_model& model : in_context.components<component_model>())
			{
				in_context.destroy_object(model.owner());
			}

			//TODO: fix crash when spamming force unload
			in_context.get_state<state_assetsystem>()->force_unload_unreferenced_assets();
		}
	}
};

int main()
{
	using namespace impuls;

	std::unique_ptr<world> new_world = std::make_unique<world>();
	new_world->set_ticksteps
	<
		tickstep_async<system_file>,
		tickstep_synced<system_asset>,
		tickstep_synced<system_window>,
		tickstep_synced<system_renderer, system_game>,
		tickstep_synced<system_renderer_state_swapper>
	>();

	new_world->initialize();

	while (!new_world->is_destroyed())
		new_world->simulate();

	return 0;
}
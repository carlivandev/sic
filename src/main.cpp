#include "pch.h"

#include "impuls/world_context.h"

#include "impuls/system_window.h"
#include "impuls/system_input.h"
#include "impuls/system_editor_view_controller.h"
#include "impuls/system_model.h"
#include "impuls/system_file.h"

#include "impuls/view.h"
#include "impuls/transform.h"
#include "impuls/asset_management.h"
#include "impuls/asset_types.h"

using namespace impuls;

struct system_game : public impuls::i_system
{
	struct player : i_object<component_transform, component_model> {};

	void on_created(world_context&& in_context) const
	{
		in_context.create_subsystem<system_window>();
		in_context.create_subsystem<system_input>();
		in_context.create_subsystem<system_model>();
		in_context.create_subsystem<system_editor_view_controller>();

		in_context.register_component_type<component_transform>("transform");
		in_context.register_object<player>("player");

		object_window& main_window = in_context.create_object_instance<object_window>();
		in_context.get_state<state_main_window>()->m_window = &main_window;

		component_view& main_view = in_context.create_object_instance<object_view>().get<component_view>();

		main_view.m_window_render_on = &main_window;
		in_context.create_object_instance<object_editor_view_controller>().get<component_editor_view_controller>().m_view_to_control = &main_view;

		//todo: split file loading from asset loading, loading texture should not require GLFW context
		glfwMakeContextCurrent(main_view.m_window_render_on->get<component_window>().m_window);

		auto tex_body = asset_management::load_texture("content/textures/crysis/body_dif.png", e_texture_format::rgb_a);
		auto tex_glass = asset_management::load_texture("content/textures/crysis/glass_dif.png", e_texture_format::rgb);
		auto tex_arm = asset_management::load_texture("content/textures/crysis/arm_dif.png", e_texture_format::rgb_a);
		auto tex_hand = asset_management::load_texture("content/textures/crysis/hand_dif.png", e_texture_format::rgb_a);
		auto tex_helmet = asset_management::load_texture("content/textures/crysis/helmet_dif.png", e_texture_format::rgb_a);
		auto tex_leg = asset_management::load_texture("content/textures/crysis/leg_dif.png", e_texture_format::rgb_a);

		auto mat_body = asset_management::load_material("content/materials/red_triangle");
		mat_body->m_texture_parameters.push_back({ "tex_albedo", tex_body });

		auto mat_glass = asset_management::load_material("content/materials/red_triangle");
		mat_glass->m_texture_parameters.push_back({ "tex_albedo", tex_glass });

		auto mat_arm = asset_management::load_material("content/materials/red_triangle");
		mat_arm->m_texture_parameters.push_back({ "tex_albedo", tex_arm });

		auto mat_hand = asset_management::load_material("content/materials/red_triangle");
		mat_hand->m_texture_parameters.push_back({ "tex_albedo", tex_hand });

		auto mat_helmet = asset_management::load_material("content/materials/red_triangle");
		mat_helmet->m_texture_parameters.push_back({ "tex_albedo", tex_helmet });

		auto mat_leg = asset_management::load_material("content/materials/red_triangle");
		mat_leg->m_texture_parameters.push_back({ "tex_albedo", tex_leg });

		auto model = asset_management::load_model("content/models/crysis.fbx");

		//create new asset with default values, also saves to disk (.asset, .asset_h)
		//auto ass = create_asset<asset_model>("asset_name", "models/subdirectory");

		//import raw data, process it
		//will not wipe data that's not changed by source data. ex. default materials
		//import_asset(ass, "content/models/crysis.fbx");

		//updates .asset and .asset_h with new values
		//save_asset(ass);

		model->set_material(mat_body, "Body");
		model->set_material(mat_glass, "Glass");
		model->set_material(mat_arm, "Arm");
		model->set_material(mat_hand, "Hand");
		model->set_material(mat_helmet, "Helmet");
		model->set_material(mat_leg, "Leg");

		{
			player& new_player = in_context.create_object_instance<player>();
			component_model& player_model = new_player.get<component_model>();
			player_model.m_model = model;

			new_player.get<component_transform>().m_position = { 0.0f, 0.0f, 0.0f };
		}

		{
			player& new_player = in_context.create_object_instance<player>();
			component_model& player_model = new_player.get<component_model>();
			player_model.m_model = model;

			new_player.get<component_transform>().m_position = { 20.0f, 0.0f, 0.0f };
		}

		in_context.get_state<state_filesystem>()->request_load
		(
			file_load_request("content/models/crysis.fbx",
				[](std::string&& in_data)
				{
					printf("loaded!\n");
				}
			)
		);

		glfwMakeContextCurrent(nullptr);
	}
};

int main()
{
	using namespace impuls;

	std::unique_ptr<world> new_world = std::make_unique<world>();
	new_world->set_ticksteps
	<
		tickstep_async<system_file>,
		tickstep_synced<system_game>
	>();

	new_world->initialize();

	while (!new_world->is_destroyed())
		new_world->simulate();

	return 0;
}
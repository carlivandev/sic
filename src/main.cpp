#include "pch.h"

#include "impuls/world_context.h"
#include "impuls/system_window.h"
#include "impuls/system_input.h"
#include "impuls/view.h"
#include "impuls/system_editor_view_controller.h"
#include "impuls/system_model.h"
#include "impuls/transform.h"
#include "impuls/asset_management.h"
#include "impuls/asset_types.h"

int main()
{
	using namespace impuls;

	struct player : i_object<component_transform, component_model> {};

	std::unique_ptr<world> new_world = std::make_unique<world>();
	world_context context(*new_world);

	new_world->create_system<system_window>();
	new_world->create_system<system_input>();
	new_world->create_system<system_model>();
	new_world->create_system<system_editor_view_controller>();

	context.register_component_type<component_transform>("transform");
	context.register_object<player>("player");

	new_world->initialize();

	object_window& main_window = context.create_object_instance<object_window>();
	context.get_state<state_main_window>()->m_window = &main_window;

	component_view& main_view = context.create_object_instance<object_view>().get<component_view>();

	main_view.m_window_render_on = &main_window;
	context.create_object_instance<object_editor_view_controller>().get<component_editor_view_controller>().m_view_to_control = &main_view;

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

	player& new_player = context.create_object_instance<player>();
	component_model& player_model = new_player.get<component_model>();
	player_model.m_model = asset_management::load_model("content/models/crysis.fbx");

	player_model.set_material(mat_body, "Body");
	player_model.set_material(mat_glass, "Glass");
	player_model.set_material(mat_arm, "Arm");
	player_model.set_material(mat_hand, "Hand");
	player_model.set_material(mat_helmet, "Helmet");
	player_model.set_material(mat_leg, "Leg");

	new_player.get<component_transform>().m_position = { 0.0f, 0.0f, 0.0f };

	while (!new_world->is_destroyed())
		new_world->simulate();

	return 0;
}
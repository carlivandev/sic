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

#include "sic/shader_parser.h"

using namespace sic;

struct System_game : public sic::System
{
	struct Component_player_move : Component
	{
		float m_t = 0.0f;
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

	void on_begin_simulation(Level_context in_context) const override
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
		auto create_test_texture = [&](const std::string& texture_name, const std::string& texture_source, Texture_format format) -> Asset_ref<Asset_texture>
		{
			auto ref = assetsystem_state->create_asset<Asset_texture>(texture_name, "content/textures/crysis");
			assetsystem_state->modify_asset<Asset_texture>
			(
				ref,
				[&texture_source, format](Asset_texture* texture)
				{
					texture->import(File_management::load_file(texture_source), format, true);
				}
			);

			assetsystem_state->save_asset<Asset_texture>(*ref.get_header());

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

					if (in_instanced)
					{
						Material_instance_data_layout layout;
						layout.add_entry<glm::mat4x4>("MVP");
						layout.add_entry<glm::mat4x4>("model_matrix");
						layout.add_entry<GLuint64>("tex_albedo");

						material->enable_instancing(1024, layout);
					}
				}
			);

			assetsystem_state->save_asset<Asset_material>(*ref.get_header());

			return ref;
		};

		auto tex_body =		(create_test_texture("tex_crysis_body", "content/textures/crysis/body_dif.png", Texture_format::rgb_a));
		auto tex_glass =	(create_test_texture("tex_crysis_glass", "content/textures/crysis/glass_dif.png", Texture_format::rgb));
		auto tex_arm =		(create_test_texture("tex_crysis_arm", "content/textures/crysis/arm_dif.png", Texture_format::rgb_a));
		auto tex_hand =		(create_test_texture("tex_crysis_hand", "content/textures/crysis/hand_dif.png", Texture_format::rgb_a));
		auto tex_helmet =	(create_test_texture("tex_crysis_helmet", "content/textures/crysis/helmet_dif.png", Texture_format::rgb_a));
		auto tex_leg =		(create_test_texture("tex_crysis_leg", "content/textures/crysis/leg_dif.png", Texture_format::rgb_a));

		bool instancing = true;

		auto mat_body =		(create_test_material("mat_crysis_body", tex_body, Material_blend_mode::Opaque, instancing));
		auto mat_glass =	(create_test_material("mat_crysis_glass", tex_glass, Material_blend_mode::Additive, instancing));
		auto mat_arm =		(create_test_material("mat_crysis_arm", tex_arm, Material_blend_mode::Opaque, instancing));
		auto mat_hand =		(create_test_material("mat_crysis_hand", tex_hand, Material_blend_mode::Opaque, instancing));
		auto mat_helmet =	(create_test_material("mat_crysis_helmet", tex_helmet, Material_blend_mode::Opaque, instancing));
		auto mat_leg =		(create_test_material("mat_crysis_leg", tex_leg, Material_blend_mode::Opaque, instancing));

		Asset_ref<Asset_material> child_material = assetsystem_state->create_asset<Asset_material>("test_child_material", "content/materials/crysis");
		assetsystem_state->modify_asset<Asset_material>
		(
			child_material,
			[&](Asset_material* material)
			{
				material->set_parent(mat_body.get_mutable());
			}
		);
		
		/*
		TODO:
		when setting parent, we are essentially saying:
		1. we want to share the same program
		2. we want to override some parameters from the parent and CANT add our own parameters

		drawcalls should have pointer to the parameters, and pointer to the OpenGl_program instead of pointer to Asset_material
		this way we can do the partition based only of the OpenGl_program pointer

		all parameters should be set per material, OR the user can do model_component->instantiate_parameters("Body");
		which will do so the model_component holds a pointer to the instance buffer in the material
		this means: the instance_buffer will be used for instancing, but also non-instanced data

		*/
		

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
		}

		{
			Player& new_player = in_context.create_object<Player>();
			Component_model& player_model = new_player.get<Component_model>();
			player_model.set_model(model_ref);

			new_player.get<Component_transform>().set_translation({ 200.0f, 0.0f, 0.0f });
			new_player.get<Component_transform>().look_at(glm::normalize(glm::vec3(50.0f, 50.0f, -200.0f) - glm::vec3(200.0f, 0.0f, 0.0f)), sic::Transform::up);
			new_player.get<Component_transform>().set_scale({ 10.0f, 20.0f, 10.0f });
		}

		for (size_t x = 0; x < 10; x++)
		{
			for (size_t y = 0; y < 100; y++)
			{
				Player& new_player = in_context.create_object<Player>();
				Component_model& player_model = new_player.get<Component_model>();
				player_model.set_model(model_ref);

				new_player.get<Component_transform>().set_translation({ x * 80.0f, y * 80.0f, 0.0f });
				new_player.get<Component_transform>().look_at(glm::normalize(glm::vec3(50.0f, 50.0f, -200.0f) - glm::vec3(200.0f, 0.0f, 0.0f)), sic::Transform::up);
				new_player.get<Component_transform>().set_scale({ 10.0f, 10.0f, 10.0f });
				new_player.get<Component_player_move>().m_t = (x + y * 0.5f) * 10.0f;
			}
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

		in_context.for_each<Component_view>
		(
			[&in_context](Component_view& in_view)
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
			in_context.for_each<Player>
			(
				[&in_context](Player&)
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
			in_context.for_each<Player>
			(
				[&in_context, in_time_delta](Player& in_player)
				{
					float& t = in_player.get<Component_player_move>().m_t;
					t += in_time_delta * 1.5f;
					in_player.get<Component_transform>().translate({ 0.0f, glm::cos(t) * 2.0f, 0.0f});
				}
			);
		}
	}
};

struct Ui_anchors
{
	void set(float in_uniform_anchors) 
	{
		m_min = { in_uniform_anchors, in_uniform_anchors };
		m_max = { in_uniform_anchors, in_uniform_anchors };
	}

	void set(float in_horizontal_anchors, float in_vertical_anchors)
	{
		m_min = { in_horizontal_anchors, in_vertical_anchors };
		m_max = { in_horizontal_anchors, in_vertical_anchors };
	}

	void set(float in_min_x, float in_min_y, float in_max_x, float in_max_y)
	{
		m_min = { in_min_x, in_min_y };
		m_max = { in_max_x, in_max_y };
	}

	const glm::vec2& get_min() const { return m_min; }
	const glm::vec2& get_max() const { return m_max; }

private:
	glm::vec2 m_min = { 0.5f, 0.5f };
	glm::vec2 m_max = { 0.5f, 0.5f };
};



struct Ui_widget
{
	void calculate_render_transform(glm::vec2& out_translation, glm::vec2& out_size, float& out_rotation);

	struct Ui_parent_widget_base* m_parent = nullptr;
	i32 m_slot_index = -1;

	glm::vec2 m_local_scale = { 1.0f, 1.0f };
	glm::vec2 m_local_translation = { 0.0f, 0.0f };
	float m_local_rotation = 0.0f;
};

struct Ui_parent_widget_base : Ui_widget
{
	virtual void calculate_render_transform_base(const Ui_widget& in_widget, glm::vec2& out_translation, glm::vec2& out_size, float& out_rotation) = 0;
};

void Ui_widget::calculate_render_transform(glm::vec2& out_translation, glm::vec2& out_size, float& out_rotation)
{
	out_translation = { 0.0f, 0.0f };
	out_size = { 1.0f, 1.0f };
	out_rotation = 0.0f;

	if (m_parent)
	{
		m_parent->calculate_render_transform_base(*this, out_translation, out_size, out_rotation);

		out_size *= m_local_scale;
		out_translation += m_local_translation;
		out_rotation += m_local_rotation;
	}
	else
	{
		/*
		if a widget doesnt have a parent, its size should cover the entire screen (1.0f, 1.0f)
		otherwise, slot types should use their data to set the correct size

		(ofcourse, locals still apply)
		*/
		out_size = m_local_scale;
		out_translation = glm::vec2(0.5f, 0.5f) + m_local_translation;
		out_rotation = m_local_rotation;
	}
}

struct Ui_slot
{
};

struct Ui_slot_canvas : Ui_slot
{
	Ui_anchors m_anchors;

	glm::vec2 m_translation;
	glm::vec2 m_size;
	glm::vec2 m_pivot;
	float m_rotation = 0;

	ui32 m_z_order = 0;
};



template <typename T_slot_type>
struct Ui_parent_widget : Ui_parent_widget_base
{
	void calculate_render_transform_base(const Ui_widget& in_widget, glm::vec2& out_translation, glm::vec2& out_size, float& out_rotation) override final
	{
		assert(in_widget.m_parent == this && "Parent mismatch!");
		calculate_render_transform(m_slots[in_widget.m_slot_index], out_translation, out_size, out_rotation);

		if (m_parent)
			m_parent->calculate_render_transform_base(in_widget, out_translation, out_size, out_rotation);
	}

	virtual void calculate_render_transform(const T_slot_type& in_slot, glm::vec2& out_translation, glm::vec2& out_size, float& out_rotation) = 0;

	template<typename T_child_type>
	void add_child(T_slot_type&& in_slot_data, T_child_type&& in_widget_data)
	{
		assert(!in_widget_data.m_parent && "Widget has already been added to some parent.");
		m_slots.emplace_back(in_slot_data);
		m_widgets.emplace_back(new T_child_type(in_widget_data));
		m_widgets.back()->m_parent = this;
		m_widgets.back()->m_slot_index = static_cast<i32>(m_slots.size() - 1);
	}

	std::vector<T_slot_type> m_slots;
	std::vector<std::unique_ptr<Ui_widget>> m_widgets;
};

struct Ui_widget_canvas : Ui_parent_widget<Ui_slot_canvas>
{
	void calculate_render_transform(const Ui_slot_canvas& in_slot, glm::vec2& out_translation, glm::vec2& out_size, float& out_rotation) override final
	{
		glm::vec2 anchor_vec = in_slot.m_anchors.get_max() - in_slot.m_anchors.get_min();
		glm::vec2 anchor_scale = anchor_vec;

		//if both anchors are 0 on all, translation should be relative to top left
		//if both anchors are 1 on all, translation should be relative to bottom right

		//if both anchors are not the same, translation should be relative to center of both
		glm::vec2 translation = in_slot.m_translation / m_reference_dimensions;
		translation += in_slot.m_anchors.get_min() + (anchor_vec * 0.5f);

		//Todo (carl): maaaybe we should support stretching with anchors, so if they are not the same we restrain the size inbetween the anchor points?

		out_size *= in_slot.m_size / m_reference_dimensions;
		out_translation += in_slot.m_translation / m_reference_dimensions;
		out_translation -= out_size * (in_slot.m_pivot);
		out_rotation += in_slot.m_rotation;
	}
	
	glm::vec2 m_reference_dimensions;
};

struct Ui_widget_image : Ui_widget
{
	Asset_ref<Asset_texture> m_texture_ref;
	glm::vec4 m_color_and_opacity = { 1.0f, 1.0f, 1.0f, 1.0f };
};

int main()
{
	using namespace sic;

	//sic::g_log_game_verbose.m_enabled = false;
	sic::g_log_asset_verbose.m_enabled = false;
	//sic::g_log_renderer_verbose.m_enabled = false;

	Ui_widget_canvas canvas;
	canvas.m_reference_dimensions = { 1920.0f, 1080.0f };
	
	Ui_slot_canvas canvas_slot;
	canvas_slot.m_anchors.set(0.0f);
	canvas_slot.m_pivot = { 0.5f, 0.5f };
	canvas_slot.m_size = { 64.0f, 64.0f }; //relative to parents reference_dimensions
	canvas_slot.m_translation = { 1920.0f / 2.0f, 1080.0f / 2.0f };
	canvas_slot.m_rotation = 0.0f;

	Ui_widget_image image;
	image.m_texture_ref = Asset_ref<Asset_texture>();
	canvas.add_child(std::move(canvas_slot), std::move(image));

	glm::vec2 render_size = { 0.0f, 0.0f };
	glm::vec2 render_translation = { 0.0f, 0.0f };
	float render_rotation = 0.0f;

	canvas.m_widgets.back()->calculate_render_transform(render_size, render_translation, render_rotation);

	

	{
		std::unique_ptr<Engine> engine_instance = std::make_unique<Engine>();

		engine_instance->add_system<System_file>(Tickstep::async);

		engine_instance->add_system<System_asset>(Tickstep::pre_tick);
		engine_instance->add_system<System_window>(Tickstep::pre_tick);
		engine_instance->add_system<System_input>(Tickstep::pre_tick);

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
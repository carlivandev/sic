#include "sic/system_renderer.h"
#include "sic/component_transform.h"

#include "sic/system_window.h"
#include "sic/component_view.h"
#include "sic/logger.h"
#include "sic/state_render_scene.h"
#include "sic/file_management.h"
#include "sic/system_asset.h"

#include "sic/opengl_vertex_buffer_array.h"
#include "sic/opengl_program.h"
#include "sic/opengl_engine_uniform_blocks.h"
#include "sic/opengl_draw_interface_debug_lines.h"
#include "sic/renderer_shape_draw_functions.h"
#include "sic/opengl_draw_strategies.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "stb/stb_image.h"

void sic::System_renderer::on_created(Engine_context in_context)
{
	in_context.register_state<State_render_scene>("state_render_scene");
	in_context.register_state<State_debug_drawing>("state_debug_drawing");
	in_context.register_state<State_renderer_resources>("State_renderer_resources");

	in_context.listen<event_created<Level>>
	(
		[](Engine_context& in_out_context, Level& in_out_level)
		{
			if (in_out_level.get_is_root_level())
			{
				State_render_scene& render_scene_state = in_out_context.get_state_checked<State_render_scene>();
				State_debug_drawing& debug_drawing_state = in_out_context.get_state_checked<State_debug_drawing>();

				//only create new render scenes for each root level
				render_scene_state.add_level(in_out_level.m_level_id);
				//only create debug drawer on root levels
				Render_object_id<Render_object_debug_drawer> obj_id = render_scene_state.create_object<Render_object_debug_drawer>(in_out_level.m_level_id, nullptr);
				debug_drawing_state.m_level_id_to_debug_drawer_ids[in_out_level.m_level_id] = obj_id;
			}
		}
	);

	in_context.listen<event_destroyed<Level>>
	(
		[](Engine_context& in_out_context, Level& in_out_level)
		{
			if (in_out_level.get_is_root_level())
			{
				State_render_scene& render_scene_state = in_out_context.get_state_checked<State_render_scene>();
				State_debug_drawing& debug_drawing_state = in_out_context.get_state_checked<State_debug_drawing>();

				auto drawer_id_it = debug_drawing_state.m_level_id_to_debug_drawer_ids.find(in_out_level.m_level_id);
				//only create debug drawer on root levels
				render_scene_state.destroy_object(drawer_id_it->second);

				debug_drawing_state.m_level_id_to_debug_drawer_ids.erase(drawer_id_it);

				//only create new render scenes for each root level
				render_scene_state.remove_level(in_out_level.m_level_id);
			}
		}
	);
}

void sic::System_renderer::on_engine_finalized(Engine_context in_context) const
{
	State_window& window_state = in_context.get_state_checked<State_window>();
	glfwMakeContextCurrent(window_state.m_resource_context);

	State_renderer_resources& resources = in_context.get_state_checked<State_renderer_resources>();

	resources.m_uniform_block_view.emplace();
	resources.m_uniform_block_lights.emplace();

	State_debug_drawing& debug_drawer_state = in_context.get_state_checked<State_debug_drawing>();
	debug_drawer_state.m_draw_interface_debug_lines.emplace(resources.m_uniform_block_view.value());

	const char* pass_through_vertex_shader_path = "content/materials/pass_through.vert";
	const char* simple_texture_fragment_shader_path = "content/materials/simple_texture.frag";

	resources.m_pass_through_program.emplace
	(
		pass_through_vertex_shader_path,
		File_management::load_file(pass_through_vertex_shader_path),
		simple_texture_fragment_shader_path,
		File_management::load_file(simple_texture_fragment_shader_path)
	);

	resources.m_quad_vertex_buffer_array.emplace();
	resources.m_quad_indexbuffer.emplace(GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW);
	const std::vector<GLfloat> positions =
	{
		-1.0f, -1.0f,
		-1.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, -1.0f
	};

	const std::vector<GLfloat> tex_coords =
	{
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f
	};

	/*
	1 2
	0 3
	*/

	std::vector<unsigned int> indices =
	{
		0, 2, 1,
		0, 3, 2
	};

	resources.m_quad_vertex_buffer_array.value().bind();

	resources.m_quad_vertex_buffer_array.value().set_data<OpenGl_vertex_attribute_position2D>(positions);
	resources.m_quad_vertex_buffer_array.value().set_data<OpenGl_vertex_attribute_texcoord>(tex_coords);

	resources.m_quad_indexbuffer.value().set_data(indices);
	
}

void sic::System_renderer::on_engine_tick(Engine_context in_context, float in_time_delta) const
{
	in_time_delta;

	State_assetsystem& assetsystem_state = in_context.get_state_checked<State_assetsystem>();
	State_window& window_state = in_context.get_state_checked<State_window>();
	State_renderer_resources& renderer_resources_state = in_context.get_state_checked<State_renderer_resources>();
	State_render_scene& scene_state = in_context.get_state_checked<State_render_scene>();

	glfwMakeContextCurrent(window_state.m_resource_context);
	
	assetsystem_state.do_post_load<Asset_texture>
	(
		[](Asset_texture& in_texture)
		{
			//do opengl load
			initialize_texture(in_texture);
		}
	);

	assetsystem_state.do_post_load<Asset_material>
	(
		[&renderer_resources_state](Asset_material& in_material)
		{
			//do opengl load
			initialize_material(renderer_resources_state, in_material);
		}
	);

	assetsystem_state.do_post_load<Asset_model>
	(
		[](Asset_model& in_model)
		{
			//do opengl load
			for (auto&& mesh : in_model.m_meshes)
				initialize_mesh(mesh);
		}
	);

	assetsystem_state.do_pre_unload<Asset_texture>
	(
		[](Asset_texture& in_texture)
		{
			in_texture.m_texture.reset();

			if (!in_texture.m_free_texture_data_after_setup)
				in_texture.m_texture_data.reset();
		}
	);

	assetsystem_state.do_pre_unload<Asset_material>
	(
		[](Asset_material& in_material)
		{
			in_material.m_program.reset();
		}
	);

	assetsystem_state.do_pre_unload<Asset_model>
	(
		[](Asset_model& in_model)
		{
			for (auto&& mesh : in_model.m_meshes)
			{
				mesh.m_vertex_buffer_array.reset();
				mesh.m_index_buffer.reset();
			}
		}
	);

	//clear all window backbuffers
	for (auto&& window : scene_state.m_windows.m_objects)
	{
		if (!window.m_context)
			continue;

		glfwMakeContextCurrent(window.m_context);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glfwMakeContextCurrent(window_state.m_resource_context);
		window.m_render_target.value().clear();
	}

	std::unordered_map<Render_object_window*, std::vector<Render_object_view*>> window_to_views_lut;
	
	for (auto&& level_to_scene_it : scene_state.m_level_id_to_scene_lut)
	{
		Update_list<Render_object_view>& views = std::get<Update_list<Render_object_view>>(level_to_scene_it.second);

		for (Render_object_view& view : views.m_objects)
		{
			Render_object_window* window = scene_state.m_windows.find_object(view.m_window_id);
			if (!window)
				continue;

			window_to_views_lut[window].push_back(&view);
		}
	}
	
	std::vector<Asset_ref<Asset_texture>> textures_to_load;
	std::vector<Asset_ref<Asset_material>> materials_to_load;
	std::vector<Asset_ref<Asset_model>> models_to_load;

	for (auto& window_to_views_it : window_to_views_lut)
	{
		glfwMakeContextCurrent(window_state.m_resource_context);

		sic::i32 current_window_x, current_window_y;
		glfwGetWindowSize(window_to_views_it.first->m_context, &current_window_x, &current_window_y);

		if (current_window_x == 0 || current_window_y == 0)
			continue;

		

		// Enable depth test
		glEnable(GL_DEPTH_TEST);
		// Accept fragment if it closer to the camera than the former one
		glDepthFunc(GL_LESS);

		glEnable(GL_CULL_FACE);
		
		for (Render_object_view* view : window_to_views_it.second)
			render_view(in_context, *window_to_views_it.first, *view, textures_to_load, materials_to_load, models_to_load);
	}
	
	render_views_to_window_backbuffers(window_to_views_lut);

	glfwMakeContextCurrent(window_state.m_resource_context);
	
	for (auto&& scene_it : scene_state.m_level_id_to_scene_lut)
	{
		Update_list<Render_object_debug_drawer>& debug_drawers = std::get<Update_list<Render_object_debug_drawer>>(scene_it.second);

		for (Render_object_debug_drawer& debug_drawer : debug_drawers.m_objects)
			debug_drawer.update_shape_lifetimes(in_time_delta);
	}

	glfwMakeContextCurrent(nullptr);

	assetsystem_state.load_batch(in_context, std::move(textures_to_load));
	assetsystem_state.load_batch(in_context, std::move(materials_to_load));
	assetsystem_state.load_batch(in_context, std::move(models_to_load));
}

void sic::System_renderer::render_view(
	Engine_context in_context, const Render_object_window& in_window, Render_object_view& inout_view,
	std::vector<Asset_ref<Asset_texture>>& out_textures_to_load,
	std::vector<Asset_ref<Asset_material>>& out_materials_to_load,
	std::vector<Asset_ref<Asset_model>>& out_models_to_load) const
{
	State_debug_drawing& debug_drawer_state = in_context.get_state_checked<State_debug_drawing>();
	State_renderer_resources& renderer_resources_state = in_context.get_state_checked<State_renderer_resources>();
	State_render_scene& scene_state = in_context.get_state_checked<State_render_scene>();

	sic::i32 current_window_x, current_window_y;
	glfwGetWindowSize(in_window.m_context, &current_window_x, &current_window_y);

	auto scene_it = scene_state.m_level_id_to_scene_lut.find(inout_view.m_level_id);

	if (scene_it == scene_state.m_level_id_to_scene_lut.end())
		return;

	const float view_aspect_ratio = (current_window_x * inout_view.m_viewport_size.x) / (current_window_y * inout_view.m_viewport_size.y);

	glm::ivec2 view_dimensions = inout_view.m_render_target.value().get_dimensions();
	const glm::ivec2 target_dimensions = { current_window_x * inout_view.m_viewport_size.x, current_window_y * inout_view.m_viewport_size.y };

	if (view_dimensions.x != target_dimensions.x ||
		view_dimensions.y != target_dimensions.y)
	{
		inout_view.m_render_target.value().resize(target_dimensions);
		view_dimensions = target_dimensions;
	}

	inout_view.m_render_target.value().bind_as_target(0);
	inout_view.m_render_target.value().clear();

	glViewport
	(
		0,
		0,
		view_dimensions.x,
		view_dimensions.y
	);

	// Set the list of draw buffers.
	GLenum draw_buffers[1] = { GL_COLOR_ATTACHMENT0 };
	SIC_GL_CHECK(glDrawBuffers(1, draw_buffers)); // "1" is the size of DrawBuffers

	const glm::mat4x4 proj_mat = glm::perspective
	(
		glm::radians(inout_view.m_fov),
		view_aspect_ratio,
		inout_view.m_near_plane,
		inout_view.m_far_plane
	);

	const glm::mat4x4 view_mat = glm::inverse(inout_view.m_view_orientation);
	const glm::mat4x4 view_proj_mat = proj_mat * view_mat;

	renderer_resources_state.m_uniform_block_view.value().set_data(0, view_mat);
	renderer_resources_state.m_uniform_block_view.value().set_data(1, proj_mat);
	renderer_resources_state.m_uniform_block_view.value().set_data(2, view_proj_mat);

	const glm::vec3 first_light_pos = glm::vec3(10.0f, 10.0f, -30.0f);

	static float cur_val = 0.0f;
	cur_val += 0.0016f;
	const glm::vec3 second_light_pos = glm::vec3(10.0f, (glm::cos(cur_val * 2.0f) * 30.0f), 0.0f);

	std::vector<OpenGl_uniform_block_light_instance> relevant_lights;
	OpenGl_uniform_block_light_instance light;
	light.m_position_and_unused = glm::vec4(first_light_pos, 0.0f);
	light.m_color_and_intensity = { 1.0f, 0.0f, 0.0f, 100.0f };

	relevant_lights.push_back(light);

	light.m_position_and_unused = glm::vec4(second_light_pos, 0.0f);
	light.m_color_and_intensity = { 0.0f, 1.0f, 0.0f, 50.0f };

	relevant_lights.push_back(light);

	const ui32 byte_size = static_cast<ui32>(sizeof(OpenGl_uniform_block_light_instance) * relevant_lights.size());

	renderer_resources_state.m_uniform_block_lights.value().set_data(0, static_cast<GLfloat>(relevant_lights.size()));
	renderer_resources_state.m_uniform_block_lights.value().set_data_raw(1, 0, byte_size, relevant_lights.data());

	const Update_list<Render_object_model> & models = std::get<Update_list<Render_object_model>>(scene_it->second);
	for (const Render_object_model& model : models.m_objects)
	{
		if (!model.m_model.is_valid())
			continue;

		if (model.m_model.get_load_state() == Asset_load_state::loaded)
		{
			Asset_model* model_asset = model.m_model.get();

			const glm::mat4 mvp = proj_mat * view_mat * model.m_orientation;

			const ui64 mesh_count = model_asset->m_meshes.size();

			for (i32 mesh_idx = 0; mesh_idx < mesh_count; mesh_idx++)
			{
				auto& mesh = model_asset->m_meshes[mesh_idx];

				auto material_override_it = model.m_material_overrides.find(mesh.m_material_slot);

				const Asset_ref<Asset_material>& mat_to_draw = material_override_it != model.m_material_overrides.end() ? material_override_it->second : model_asset->get_material(mesh_idx);
				if (!mat_to_draw.is_valid())
					continue;

				if (mat_to_draw.get_load_state() == Asset_load_state::loaded)
				{
					bool all_texture_loaded = true;

					for (Asset_material::Texture_parameter& texture_param : mat_to_draw.get()->m_texture_parameters)
					{
						if (texture_param.m_texture.is_valid())
						{
							if (texture_param.m_texture.get_load_state() == Asset_load_state::not_loaded)
								out_textures_to_load.push_back(texture_param.m_texture);

							if (texture_param.m_texture.get_load_state() != Asset_load_state::loaded)
								all_texture_loaded = false;
						}
					}

					if (all_texture_loaded)
						render_mesh(mesh, *mat_to_draw.get(), mvp, model.m_orientation);
				}
				else if (mat_to_draw.get_load_state() == Asset_load_state::not_loaded)
				{
					out_materials_to_load.push_back(mat_to_draw);
				}
			}
		}
		else if (model.m_model.get_load_state() == Asset_load_state::not_loaded)
			out_models_to_load.push_back(model.m_model);
	}

	debug_drawer_state.m_draw_interface_debug_lines.value().begin_frame();

	Update_list<Render_object_debug_drawer>& debug_drawers = std::get<Update_list<Render_object_debug_drawer>>(scene_it->second);

	for (Render_object_debug_drawer& debug_drawer : debug_drawers.m_objects)
		debug_drawer.draw_shapes(debug_drawer_state.m_draw_interface_debug_lines.value());

	debug_drawer_state.m_draw_interface_debug_lines.value().end_frame();

	in_window.m_render_target.value().bind_as_target(0);

	SIC_GL_CHECK(glViewport
	(
		static_cast<GLsizei>((current_window_x * inout_view.m_viewport_offset.x)),
		static_cast<GLsizei>(current_window_y * inout_view.m_viewport_offset.y),
		static_cast<GLsizei>((current_window_x * inout_view.m_viewport_size.x)),
		static_cast<GLsizei>(current_window_y * inout_view.m_viewport_size.y)
	));

	renderer_resources_state.m_pass_through_program.value().use();

	GLuint tex_loc_id = renderer_resources_state.m_pass_through_program.value().get_uniform_location("uniform_texture");

	inout_view.m_render_target.value().bind_as_texture(tex_loc_id, 0);

	renderer_resources_state.m_quad_vertex_buffer_array.value().bind();
	renderer_resources_state.m_quad_indexbuffer.value().bind();

	OpenGl_draw_strategy_triangle_element::draw(static_cast<GLsizei>(renderer_resources_state.m_quad_indexbuffer.value().get_max_elements()), 0);
}

void sic::System_renderer::render_mesh(const Asset_model::Mesh& in_mesh, const Asset_material& in_material, const glm::mat4& in_mvp, const glm::mat4& in_model_matrix) const
{
	auto& program = in_material.m_program.value();
	program.use();

	program.set_uniform("MVP", in_mvp);
	program.set_uniform("model_matrix", in_model_matrix);

	ui32 texture_param_idx = 0;

	for (auto& texture_param : in_material.m_texture_parameters)
	{
		//in future we want to render an error texture instead
		if (!texture_param.m_texture.is_valid())
			continue;

		const i32 uniform_loc = in_material.m_program.value().get_uniform_location(texture_param.m_name.c_str());

		//error handle this
		if (uniform_loc == -1)
		{
			SIC_LOG_E(g_log_renderer_verbose,
				"Texture parameter: {0}, not found in material with shaders: {1} and {2}",
				texture_param.m_name.c_str(), in_material.m_vertex_shader_path.c_str(), in_material.m_fragment_shader_path.c_str());
			continue;
		}

		texture_param.m_texture.get()->m_texture.value().bind(uniform_loc, texture_param_idx);
		++texture_param_idx;
	}

	// draw mesh
	in_mesh.m_vertex_buffer_array.value().bind();
	in_mesh.m_index_buffer.value().bind();

	OpenGl_draw_strategy_triangle_element::draw(static_cast<GLsizei>(in_mesh.m_index_buffer.value().get_max_elements()), 0);

	glActiveTexture(GL_TEXTURE0);
}

void sic::System_renderer::render_views_to_window_backbuffers(const std::unordered_map<sic::Render_object_window*, std::vector<sic::Render_object_view*>>& in_window_to_view_lut) const
{
	for (auto& window_to_views_it : in_window_to_view_lut)
	{
		glfwMakeContextCurrent(window_to_views_it.first->m_context);
		window_to_views_it.first->draw_to_backbuffer();

		glfwSwapBuffers(window_to_views_it.first->m_context);
	}
}

void sic::System_renderer::initialize_texture(Asset_texture& out_texture)
{
	i32 gl_texture_format = 0;

	switch (out_texture.m_format)
	{
	case Texture_format::gray:
		gl_texture_format = GL_R;
		break;
	case Texture_format::gray_a:
		gl_texture_format = GL_RG;
		break;
	case Texture_format::rgb:
		gl_texture_format = GL_RGB;
		break;
	case Texture_format::rgb_a:
		gl_texture_format = GL_RGBA;
		break;
	default:
		break;
	}

	out_texture.m_texture.emplace(glm::ivec2(out_texture.m_width, out_texture.m_height), gl_texture_format, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, out_texture.m_texture_data.get());

	if (out_texture.m_free_texture_data_after_setup)
		out_texture.m_texture_data.reset();
}

void sic::System_renderer::initialize_material(const State_renderer_resources& in_resource_state, Asset_material& out_material)
{
	out_material.m_program.emplace(out_material.m_vertex_shader_path, out_material.m_vertex_shader_code, out_material.m_fragment_shader_path, out_material.m_fragment_shader_code);

	out_material.m_program.value().set_uniform_block(in_resource_state.m_uniform_block_view.value());
	out_material.m_program.value().set_uniform_block(in_resource_state.m_uniform_block_lights.value());
}

void sic::System_renderer::initialize_mesh(Asset_model::Mesh& inout_mesh)
{
	auto& vertex_buffer_array = inout_mesh.m_vertex_buffer_array.emplace();
	vertex_buffer_array.bind();

	vertex_buffer_array.set_data<OpenGl_vertex_attribute_position3D>(inout_mesh.m_positions);
	vertex_buffer_array.set_data<OpenGl_vertex_attribute_normal>(inout_mesh.m_normals);
	vertex_buffer_array.set_data<OpenGl_vertex_attribute_texcoord>(inout_mesh.m_texcoords);
	vertex_buffer_array.set_data<OpenGl_vertex_attribute_tangent>(inout_mesh.m_tangents);
	vertex_buffer_array.set_data<OpenGl_vertex_attribute_bitangent>(inout_mesh.m_bitangents);

	auto& index_buffer = inout_mesh.m_index_buffer.emplace(GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW);
	index_buffer.set_data(inout_mesh.m_indices);
}

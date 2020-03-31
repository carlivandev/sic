#include "sic/system_renderer.h"
#include "sic/component_transform.h"

#include "sic/asset_types.h"
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

namespace sic_private
{
	using namespace sic;

	void init_texture(Asset_texture& out_texture)
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

	void init_material(Asset_material& out_material)
	{
		out_material.m_program.emplace(out_material.m_vertex_shader_path, out_material.m_vertex_shader_code, out_material.m_fragment_shader_path, out_material.m_fragment_shader_code);

		out_material.m_program.value().set_uniform_block(OpenGl_uniform_block_view::get());
		out_material.m_program.value().set_uniform_block(OpenGl_uniform_block_lights::get());
	}

	void init_mesh(Asset_model::Mesh& inout_mesh)
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

	void draw_mesh(const Asset_model::Mesh& in_mesh, const Asset_material& in_material, const glm::mat4& in_mvp, const glm::mat4& in_model_matrix)
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
}

void sic::System_renderer::on_created(Engine_context in_context)
{
	in_context.register_state<State_render_scene>("state_render_scene");
	in_context.register_state<State_debug_drawing>("state_debug_drawing");
	
	in_context.listen<event_created<Level>>
	(
		[](Engine_context& in_out_context, Level& in_out_level)
		{
			if (in_out_level.get_is_root_level())
			{
				State_render_scene* render_scene_state = in_out_context.get_state<State_render_scene>();
				State_debug_drawing* debug_drawing_state = in_out_context.get_state<State_debug_drawing>();

				//only create new render scenes for each root level
				render_scene_state->add_level(in_out_level.m_level_id);
				//only create debug drawer on root levels
				Render_object_id<Render_object_debug_drawer> obj_id = render_scene_state->create_object<Render_object_debug_drawer>(in_out_level.m_level_id, nullptr);
				debug_drawing_state->m_level_id_to_debug_drawer_ids[in_out_level.m_level_id] = obj_id;
			}
		}
	);

	in_context.listen<event_destroyed<Level>>
	(
		[](Engine_context& in_out_context, Level& in_out_level)
		{
			if (in_out_level.get_is_root_level())
			{
				State_render_scene* render_scene_state = in_out_context.get_state<State_render_scene>();
				State_debug_drawing* debug_drawing_state = in_out_context.get_state<State_debug_drawing>();

				auto drawer_id_it = debug_drawing_state->m_level_id_to_debug_drawer_ids.find(in_out_level.m_level_id);
				//only create debug drawer on root levels
				render_scene_state->destroy_object(drawer_id_it->second);

				debug_drawing_state->m_level_id_to_debug_drawer_ids.erase(drawer_id_it);

				//only create new render scenes for each root level
				render_scene_state->remove_level(in_out_level.m_level_id);
			}
		}
	);
}

void sic::System_renderer::on_engine_tick(Engine_context in_context, float in_time_delta) const
{
	in_time_delta;

	State_assetsystem* assetsystem_state = in_context.get_state<State_assetsystem>();

	if (!assetsystem_state)
		return;

	State_window* window_state = in_context.get_state<State_window>();
	
	glfwMakeContextCurrent(window_state->m_resource_context);
	
	assetsystem_state->do_post_load<Asset_texture>
	(
		[](Asset_texture& in_texture)
		{
			//do opengl load
			sic_private::init_texture(in_texture);
		}
	);

	assetsystem_state->do_post_load<Asset_material>
	(
		[](Asset_material& in_material)
		{
			//do opengl load
			sic_private::init_material(in_material);
		}
	);

	assetsystem_state->do_post_load<Asset_model>
	(
		[](Asset_model& in_model)
		{
			//do opengl load
			for (auto&& mesh : in_model.m_meshes)
				sic_private::init_mesh(mesh);
		}
	);

	assetsystem_state->do_pre_unload<Asset_texture>
	(
		[](Asset_texture& in_texture)
		{
			in_texture.m_texture.reset();

			if (!in_texture.m_free_texture_data_after_setup)
				in_texture.m_texture_data.reset();
		}
	);

	assetsystem_state->do_pre_unload<Asset_material>
	(
		[](Asset_material& in_material)
		{
			in_material.m_program.reset();
		}
	);

	assetsystem_state->do_pre_unload<Asset_model>
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

	State_render_scene* scene_state = in_context.get_state<State_render_scene>();

	if (!scene_state)
		return;

	std::vector<Asset_ref<Asset_texture>> textures_to_load;
	std::vector<Asset_ref<Asset_material>> materials_to_load;
	std::vector<Asset_ref<Asset_model>> models_to_load;

	std::unordered_map<GLFWwindow*, std::vector<Render_object_view*>> window_to_views_lut;

	static std::unordered_map<GLFWwindow*, std::unique_ptr<Render_object_window>> context_to_window_data_lut;
	
	for (auto&& level_to_scene_it : scene_state->m_level_id_to_scene_lut)
	{
		Update_list<Render_object_view>& views = std::get<Update_list<Render_object_view>>(level_to_scene_it.second);

		for (Render_object_view& view : views.m_objects)
		{
			if (!view.m_window_render_on)
				continue;

			auto window_data_it = context_to_window_data_lut.find(view.m_window_render_on);

			if (window_data_it == context_to_window_data_lut.end())
			{
				glfwMakeContextCurrent(view.m_window_render_on);
				context_to_window_data_lut[view.m_window_render_on] = std::make_unique<Render_object_window>(view.m_window_render_on);
			}

			//we have to initialize fbo on main context cause it is not shared
			glfwMakeContextCurrent(window_state->m_resource_context);

			sic::i32 current_window_x, current_window_y;
			glfwGetWindowSize(view.m_window_render_on, &current_window_x, &current_window_y);

			auto& window_data = context_to_window_data_lut[view.m_window_render_on];
			if (window_data->m_render_target.has_value())
			{
				if (window_data->m_render_target.value().get_dimensions().x != current_window_x ||
					window_data->m_render_target.value().get_dimensions().y != current_window_y)
				{
					window_data->m_render_target.value().resize({ current_window_x, current_window_y });
				}
			}
			else
			{
				window_data->m_render_target.emplace(glm::ivec2{ current_window_x, current_window_y }, false);
			}

			window_to_views_lut[view.m_window_render_on].push_back(&view);
		}
	}

	for (auto& window_to_views_it : window_to_views_lut)
	{
		glfwMakeContextCurrent(window_to_views_it.first);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glfwMakeContextCurrent(window_state->m_resource_context);
		auto& window_data = context_to_window_data_lut[window_to_views_it.first];
		window_data->m_render_target.value().clear();
	}
	
	for (auto& window_to_views_it : window_to_views_lut)
	{
		GLFWwindow* render_window = window_to_views_it.first;

		glfwMakeContextCurrent(window_state->m_resource_context);

		// Enable depth test
		glEnable(GL_DEPTH_TEST);
		// Accept fragment if it closer to the camera than the former one
		glDepthFunc(GL_LESS);

		glEnable(GL_CULL_FACE);

		sic::i32 current_window_x, current_window_y;
		glfwGetWindowSize(render_window, &current_window_x, &current_window_y);

		if (current_window_x == 0 || current_window_y == 0)
			continue;
		
		for (Render_object_view* view : window_to_views_it.second)
		{
			GLenum err;

			auto scene_it = scene_state->m_level_id_to_scene_lut.find(view->m_level_id);

			if (scene_it == scene_state->m_level_id_to_scene_lut.end())
				continue;

			const float view_aspect_ratio = (current_window_x * view->m_viewport_size.x) / (current_window_y * view->m_viewport_size.y);

			glm::ivec2 view_dimensions = view->m_render_target.value().get_dimensions();
			const glm::ivec2 target_dimensions = { current_window_x * view->m_viewport_size.x, current_window_y * view->m_viewport_size.y };

			if (view_dimensions.x != target_dimensions.x ||
				view_dimensions.y != target_dimensions.y)
			{
				view->m_render_target.value().resize(target_dimensions);
				view_dimensions = target_dimensions;
			}

			view->m_render_target.value().bind_as_target(0);
			view->m_render_target.value().clear();

			glViewport
			(
				0,
				0,
				view_dimensions.x,
				view_dimensions.y
			);

			// Set the list of draw buffers.
			GLenum draw_buffers[1] = { GL_COLOR_ATTACHMENT0 };
			glDrawBuffers(1, draw_buffers); // "1" is the size of DrawBuffers

			while ((err = glGetError()) != GL_NO_ERROR)
				SIC_LOG_E(g_log_renderer, "OpenGL error: {0}", gluErrorString(err));

			const glm::mat4x4 proj_mat = glm::perspective
			(
				glm::radians(view->m_fov),
				view_aspect_ratio,
				view->m_near_plane,
				view->m_far_plane
			);

			const glm::mat4x4 view_mat = glm::inverse(view->m_view_orientation);
			const glm::mat4x4 view_proj_mat = proj_mat * view_mat;

			OpenGl_uniform_block_view::get().set_data(0, view_mat);
			OpenGl_uniform_block_view::get().set_data(1, proj_mat);
			OpenGl_uniform_block_view::get().set_data(2, view_proj_mat);

			while ((err = glGetError()) != GL_NO_ERROR)
				SIC_LOG_E(g_log_renderer, "OpenGL error: {0}", gluErrorString(err));

			const glm::vec3 first_light_pos = glm::vec3(10.0f, 10.0f, -30.0f);

			static float cur_val = 0.0f;
			cur_val += in_time_delta;
			const glm::vec3 second_light_pos = glm::vec3(10.0f, (glm::cos(cur_val * 2.0f) * 30.0f), 0.0f);

			std::vector<OpenGl_uniform_block_light_instance> relevant_lights;
			OpenGl_uniform_block_light_instance light;
			light.m_position_and_unused = glm::vec4(first_light_pos, 0.0f);
			light.m_color_and_intensity = { 1.0f, 0.0f, 0.0f, 100.0f };

			relevant_lights.push_back(light);

			light.m_position_and_unused = glm::vec4(second_light_pos, 0.0f);
			light.m_color_and_intensity = { 0.0f, 1.0f, 0.0f, 50.0f };

			relevant_lights.push_back(light);

			static OpenGl_draw_interface_debug_lines draw_interface_debug_lines;
			draw_interface_debug_lines.begin_frame();

			sic::renderer_shape_draw_functions::draw_line(draw_interface_debug_lines, second_light_pos, first_light_pos, light.m_color_and_intensity);
			sic::renderer_shape_draw_functions::draw_sphere(draw_interface_debug_lines, light.m_position_and_unused, 32.0f, 16, light.m_color_and_intensity);

			sic::renderer_shape_draw_functions::draw_cube(draw_interface_debug_lines, light.m_position_and_unused, glm::vec3(32.0f, 32.0f, 32.0f), glm::quat(glm::vec3(0.0f, 0.0f, cur_val)), light.m_color_and_intensity);

			sic::renderer_shape_draw_functions::draw_cone(draw_interface_debug_lines, first_light_pos, glm::normalize(second_light_pos - first_light_pos), 32.0f, glm::radians(45.0f), glm::radians(45.0f), 16, light.m_color_and_intensity);

			sic::renderer_shape_draw_functions::draw_capsule(draw_interface_debug_lines, first_light_pos, 20.0f, 10.0f, glm::quat(glm::vec3(0.0f, 0.0f, cur_val)), glm::vec4(1.0f, 0.0f, 1.0f, 1.0f));

			for (size_t i = 0; i < 100; i++)
			{
				sic::renderer_shape_draw_functions::draw_capsule(draw_interface_debug_lines, glm::vec3(i * 10.0f, 0.0f, 0.0f), 20.0f, 10.0f, glm::quat(glm::vec3(0.0f, 0.0f, cur_val)), glm::vec4(1.0f, 0.0f, 1.0f, 1.0f));
			}

			for (Render_object_view* debug_draw_view : window_to_views_it.second)
			{
				if (view == debug_draw_view)
					continue;

				const float debug_draw_view_aspect_ratio = (current_window_x * debug_draw_view->m_viewport_size.x) / (current_window_y * debug_draw_view->m_viewport_size.y);

				const glm::mat4x4 debug_draw_proj_mat = glm::perspective
				(
					glm::radians(debug_draw_view->m_fov),
					debug_draw_view_aspect_ratio,
					debug_draw_view->m_near_plane,
					debug_draw_view->m_far_plane
				);
				
				const glm::mat4x4 debug_draw_view_frustum_to_world = debug_draw_view->m_view_orientation * glm::inverse(debug_draw_proj_mat);
				sic::renderer_shape_draw_functions::draw_frustum(draw_interface_debug_lines, debug_draw_view_frustum_to_world, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
			}
			
			const ui32 byte_size = static_cast<ui32>(sizeof(OpenGl_uniform_block_light_instance) * relevant_lights.size());

			OpenGl_uniform_block_lights::get().set_data(0, static_cast<GLfloat>(relevant_lights.size()));
			OpenGl_uniform_block_lights::get().set_data_raw(1, 0, byte_size, relevant_lights.data());

			const Update_list<Render_object_model>& models = std::get<Update_list<Render_object_model>>(scene_it->second);
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
										textures_to_load.push_back(texture_param.m_texture);

									if (texture_param.m_texture.get_load_state() != Asset_load_state::loaded)
										all_texture_loaded = false;
								}
							}

							if (all_texture_loaded)
								sic_private::draw_mesh(mesh, *mat_to_draw.get(), mvp, model.m_orientation);
						}
						else if (mat_to_draw.get_load_state() == Asset_load_state::not_loaded)
						{
							materials_to_load.push_back(mat_to_draw);
						}
					}
				}
				else if (model.m_model.get_load_state() == Asset_load_state::not_loaded)
					models_to_load.push_back(model.m_model);
			}

			Update_list<Render_object_debug_drawer>& debug_drawers = std::get<Update_list<Render_object_debug_drawer>>(scene_it->second);
			for (Render_object_debug_drawer& debug_drawer : debug_drawers.m_objects)
			{
				debug_drawer.draw_shapes(draw_interface_debug_lines, in_time_delta / static_cast<float>(window_to_views_it.second.size()));
			}

			draw_interface_debug_lines.end_frame();

			
			static bool one_shot = false;

			const std::string quad_vertex_shader_path = "content/materials/pass_through.vert";
			const std::string quad_fragment_shader_path = "content/materials/simple_texture.frag";
			static OpenGl_program quad_program(quad_vertex_shader_path, File_management::load_file(quad_vertex_shader_path), quad_fragment_shader_path, File_management::load_file(quad_fragment_shader_path));

			static OpenGl_vertex_buffer_array
				<
				OpenGl_vertex_attribute_position2D,
				OpenGl_vertex_attribute_texcoord
				> quad_vertex_buffer_array;

			static OpenGl_buffer quad_indexbuffer(GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW);

			if (!one_shot)
			{
				one_shot = true;

				const std::vector<GLfloat> positions = {
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

				quad_vertex_buffer_array.bind();

				quad_vertex_buffer_array.set_data<OpenGl_vertex_attribute_position2D>(positions);
				quad_vertex_buffer_array.set_data<OpenGl_vertex_attribute_texcoord>(tex_coords);

				quad_indexbuffer.set_data(indices);
			}

			context_to_window_data_lut[render_window]->m_render_target.value().bind_as_target(0);

			while ((err = glGetError()) != GL_NO_ERROR)
				SIC_LOG_E(g_log_renderer, "OpenGL error: {0}", gluErrorString(err));

			glViewport
			(
				static_cast<GLsizei>((current_window_x * view->m_viewport_offset.x)),
				static_cast<GLsizei>(current_window_y * view->m_viewport_offset.y),
				static_cast<GLsizei>((current_window_x * view->m_viewport_size.x)),
				static_cast<GLsizei>(current_window_y * view->m_viewport_size.y)
			);

			while ((err = glGetError()) != GL_NO_ERROR)
				SIC_LOG_E(g_log_renderer, "OpenGL error: {0}", gluErrorString(err));

			quad_program.use();

			GLuint tex_loc_id = quad_program.get_uniform_location("uniform_texture");

			view->m_render_target.value().bind_as_texture(tex_loc_id, 0);

			quad_vertex_buffer_array.bind();
			quad_indexbuffer.bind();

			OpenGl_draw_strategy_triangle_element::draw(static_cast<GLsizei>(quad_indexbuffer.get_max_elements()), 0);
		}
	}
	
	for (auto& window_to_views_it : window_to_views_lut)
	{
		glfwMakeContextCurrent(window_to_views_it.first);
		context_to_window_data_lut[window_to_views_it.first]->draw_to_backbuffer();

		glfwSwapBuffers(window_to_views_it.first);
	}

	glfwMakeContextCurrent(nullptr);

	assetsystem_state->load_batch(in_context, std::move(textures_to_load));
	assetsystem_state->load_batch(in_context, std::move(materials_to_load));
	assetsystem_state->load_batch(in_context, std::move(models_to_load));
}
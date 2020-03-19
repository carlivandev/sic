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

		out_texture.m_texture.emplace(glm::ivec2(out_texture.m_width, out_texture.m_height), gl_texture_format, out_texture.m_texture_data.get());

		if (out_texture.m_free_texture_data_after_setup)
			out_texture.m_texture_data.reset();
	}

	void init_material(Asset_material& out_material)
	{
		out_material.m_program.emplace(out_material.m_vertex_shader_path, out_material.m_vertex_shader_code, out_material.m_fragment_shader_path, out_material.m_fragment_shader_code);
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

	void draw_mesh(const Asset_model::Mesh& in_mesh, const Asset_material& in_material, const glm::mat4& in_mvp)
	{
		// Use our shader
		in_material.m_program.value().use();

		GLuint mvp_uniform_loc = in_material.m_program.value().get_uniform_location("MVP");

		if (mvp_uniform_loc == -1)
			return;

		glUniformMatrix4fv(mvp_uniform_loc, 1, GL_FALSE, glm::value_ptr(in_mvp));

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
		glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(in_mesh.m_index_buffer.value().size()), GL_UNSIGNED_INT, 0);

		glActiveTexture(GL_TEXTURE0);
	}
}

void sic::System_renderer::on_created(Engine_context&& in_context)
{
	in_context.register_state<State_render_scene>("state_render_scene");
}

void sic::System_renderer::on_engine_tick(Engine_context&& in_context, float in_time_delta) const
{
	in_time_delta;

	State_assetsystem* assetsystem_state = in_context.get_state<State_assetsystem>();

	if (!assetsystem_state)
		return;

	State_window* window_state = in_context.get_state<State_window>();

	glfwMakeContextCurrent(window_state->m_main_window->get<Component_window>().m_window);
	
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

	for (Render_object_view& view : scene_state->m_views.m_objects)
	{
		if (!view.m_window_render_on)
			continue;

		window_to_views_lut[view.m_window_render_on].push_back(&view);
	}

	for (auto& window_to_views_it : window_to_views_lut)
	{
		glfwMakeContextCurrent(window_to_views_it.first);

		sic::i32 current_window_x, current_window_y;
		glfwGetWindowSize(window_to_views_it.first, &current_window_x, &current_window_y);

		if (current_window_x == 0 || current_window_y == 0)
			continue;

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		const float aspect_ratio = static_cast<float>(current_window_x) / static_cast<float>(current_window_y);

		for (Render_object_view* view : window_to_views_it.second)
		{
			auto scene_it = scene_state->m_level_id_to_scene_lut.find(view->m_level_id);

			if (scene_it == scene_state->m_level_id_to_scene_lut.end())
				continue;

			view->m_render_target.value().bind_as_target(0);
			view->m_render_target.value().clear();
			const glm::ivec2& view_dimensions = view->m_render_target.value().get_dimensions();

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

			const glm::mat4x4 proj_mat = glm::perspective
			(
				glm::radians(view->m_fov),
				aspect_ratio,
				view->m_near_plane,
				view->m_far_plane
			);

			const glm::mat4x4 view_mat = glm::inverse(view->m_view_orientation);

			const Update_list<Render_object_model>& models = std::get<Update_list<Render_object_model>>(scene_it->second);
			for (const Render_object_model& model : models.m_objects)
			{
				if (!model.m_model.is_valid())
					continue;

				if (model.m_model.get_load_state() == Asset_load_state::loaded)
				{
					Asset_model* model_asset = model.m_model.get();

					const glm::mat4 model_mat = glm::mat4(
						1.0f, 0.0f, 0.0f, 0.0f,
						0.0f, 1.0f, 0.0f, 0.0f,
						0.0f, 0.0f, 1.0f, 0.0f,
						model.m_position.x, model.m_position.y, model.m_position.z, 1.0);

					const glm::mat4 mvp = proj_mat * view_mat * model_mat;

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
								sic_private::draw_mesh(mesh, *mat_to_draw.get(), mvp);
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

			static bool one_shot = false;

			const std::string quad_vertex_shader_path = "content/materials/pass_through.vs";
			const std::string quad_fragment_shader_path = "content/materials/simple_texture.fs";
			static OpenGl_program quad_program(quad_vertex_shader_path, File_management::load_file(quad_vertex_shader_path), quad_fragment_shader_path, File_management::load_file(quad_fragment_shader_path));

			static OpenGl_vertex_buffer_array
				<
				OpenGl_vertex_attribute_position2D,
				OpenGl_vertex_attribute_texcoord
				> quad_vertex_buffer_array;

			static OpenGl_buffer quad_indexbuffer(GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW);

			static std::vector<unsigned int> indices;

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
				indices = {
					0, 2, 1,
					0, 3, 2
				};

				quad_vertex_buffer_array.bind();

				quad_vertex_buffer_array.set_data<OpenGl_vertex_attribute_position2D>(positions);
				quad_vertex_buffer_array.set_data<OpenGl_vertex_attribute_texcoord>(tex_coords);

				quad_indexbuffer.set_data(indices);
			}

			// render to  backbuffer
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			glViewport
			(
				static_cast<GLsizei>((current_window_x * view->m_viewport_offset.x)),
				static_cast<GLsizei>(current_window_y * view->m_viewport_offset.y),
				static_cast<GLsizei>((current_window_x * view->m_viewport_size.x)),
				static_cast<GLsizei>(current_window_y * view->m_viewport_size.y)
			);

			quad_program.use();
			GLuint tex_loc_id = quad_program.get_uniform_location("uniform_texture");

			view->m_render_target.value().bind_as_texture(0, tex_loc_id);

			quad_vertex_buffer_array.bind();
			quad_indexbuffer.bind();

			glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(quad_indexbuffer.size()), GL_UNSIGNED_INT, 0);
		}

		glfwSwapBuffers(window_to_views_it.first);
	}

	glfwMakeContextCurrent(nullptr);

	assetsystem_state->load_batch(in_context, std::move(textures_to_load));
	assetsystem_state->load_batch(in_context, std::move(materials_to_load));
	assetsystem_state->load_batch(in_context, std::move(models_to_load));
}
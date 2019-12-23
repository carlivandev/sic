#include "sic/system_renderer.h"
#include "sic/component_transform.h"

#include "sic/asset_types.h"
#include "sic/system_window.h"
#include "sic/component_view.h"
#include "sic/logger.h"
#include "sic/state_render_scene.h"
#include "sic/file_management.h"
#include "sic/system_asset.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "stb/stb_image.h"

namespace impuls_private
{
	using namespace sic;

	void init_texture(Asset_texture& out_texture)
	{
		GLuint& texture_id = out_texture.m_render_id;

		glGenTextures(1, &texture_id);

		glBindTexture(GL_TEXTURE_2D, texture_id);

		// Nice trilinear filtering.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// When MAGnifying the image (no bigger mipmap available), use LINEAR filtering
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// When MINifying the image, use a LINEAR blend of two mipmaps, each filtered LINEARLY too
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

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

		glTexImage2D(GL_TEXTURE_2D, 0, gl_texture_format, out_texture.m_width, out_texture.m_height, 0, gl_texture_format, GL_UNSIGNED_BYTE, out_texture.m_texture_data.get());

		glGenerateMipmap(GL_TEXTURE_2D);

		glBindTexture(GL_TEXTURE_2D, 0);

		if (out_texture.m_free_texture_data_after_setup)
			out_texture.m_texture_data.reset();

		//end gpu texture setup
	}

	void init_material(Asset_material& out_material)
	{
		const std::string vertex_file_path = out_material.m_vertex_shader;
		const std::string fragment_file_path = out_material.m_fragment_shader;

		// Create the shaders
		GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
		GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

		const std::string vertex_shader_code = File_management::load_file(vertex_file_path, false);

		if (vertex_shader_code.size() == 0)
		{
			SIC_LOG_E(g_log_renderer_verbose, "Impossible to open {0}. Are you in the right directory?", vertex_file_path.c_str());
			return;
		}
		const std::string fragment_shader_code = File_management::load_file(fragment_file_path, false);

		if (fragment_shader_code.size() == 0)
		{
			SIC_LOG_E(g_log_renderer_verbose, "Impossible to open {0}. Are you in the right directory?", fragment_file_path.c_str());
			return;
		}

		GLint result_id = GL_FALSE;
		i32 info_log_length;

		// Compile Vertex Shader
		SIC_LOG(g_log_renderer_verbose, "Compiling shader : {0}", vertex_file_path.c_str());

		const GLchar* vertex_source_ptr = vertex_shader_code.data();
		glShaderSource(vertex_shader_id, 1, &vertex_source_ptr, NULL);
		glCompileShader(vertex_shader_id);

		// Check Vertex Shader
		glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &result_id);
		glGetShaderiv(vertex_shader_id, GL_INFO_LOG_LENGTH, &info_log_length);
		if (info_log_length > 0)
		{
			std::vector<char> vertex_shader_error_message(static_cast<size_t>(info_log_length) + 1);
			glGetShaderInfoLog(vertex_shader_id, info_log_length, NULL, &vertex_shader_error_message[0]);
			SIC_LOG(g_log_renderer_verbose, &vertex_shader_error_message[0]);
		}

		// Compile Fragment Shader
		SIC_LOG(g_log_renderer_verbose, "Compiling shader : {0}", fragment_file_path.c_str());

		const GLchar* fragment_source_ptr = fragment_shader_code.data();
		glShaderSource(fragment_shader_id, 1, &fragment_source_ptr, NULL);
		glCompileShader(fragment_shader_id);

		// Check Fragment Shader
		glGetShaderiv(fragment_shader_id, GL_COMPILE_STATUS, &result_id);
		glGetShaderiv(fragment_shader_id, GL_INFO_LOG_LENGTH, &info_log_length);
		if (info_log_length > 0)
		{
			std::vector<char> fragment_shader_error_message(static_cast<size_t>(info_log_length) + 1);
			glGetShaderInfoLog(fragment_shader_id, info_log_length, NULL, &fragment_shader_error_message[0]);
			SIC_LOG_E(g_log_renderer_verbose, &fragment_shader_error_message[0]);
		}

		// Link the program
		SIC_LOG(g_log_renderer_verbose, "Linking program");
		GLuint program_id = glCreateProgram();
		glAttachShader(program_id, vertex_shader_id);
		glAttachShader(program_id, fragment_shader_id);
		glLinkProgram(program_id);

		// Check the program
		glGetProgramiv(program_id, GL_LINK_STATUS, &result_id);
		glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &info_log_length);
		if (info_log_length > 0)
		{
			std::vector<char> program_error_message(static_cast<size_t>(info_log_length) + 1);
			glGetProgramInfoLog(program_id, info_log_length, NULL, &program_error_message[0]);
			SIC_LOG_E(g_log_renderer_verbose, &program_error_message[0]);
		}

		glDetachShader(program_id, vertex_shader_id);
		glDetachShader(program_id, fragment_shader_id);

		glDeleteShader(vertex_shader_id);
		glDeleteShader(fragment_shader_id);

		out_material.m_program_id = program_id;
	}

	void init_mesh(Asset_model::Mesh& out_mesh)
	{
		std::vector<Asset_model::Asset_model::Mesh::Vertex>& vertices = out_mesh.m_vertices;
		std::vector<ui32>& indices = out_mesh.m_indices;

		glGenVertexArrays(1, &out_mesh.m_vao);
		glGenBuffers(1, &out_mesh.m_vbo);
		glGenBuffers(1, &out_mesh.m_ebo);

		glBindVertexArray(out_mesh.m_vao);
		glBindBuffer(GL_ARRAY_BUFFER, out_mesh.m_vbo);

		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Asset_model::Mesh::Vertex), &vertices[0], GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, out_mesh.m_ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(ui32), &indices[0], GL_STATIC_DRAW);

		// vertex positions
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Asset_model::Mesh::Vertex), (void*)0);
		// vertex normals
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Asset_model::Mesh::Vertex), (void*)offsetof(Asset_model::Mesh::Vertex, m_normal));
		// vertex texture coords
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Asset_model::Mesh::Vertex), (void*)offsetof(Asset_model::Mesh::Vertex, m_tex_coords));
		// vertex tangent
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Asset_model::Mesh::Vertex), (void*)offsetof(Asset_model::Mesh::Vertex, m_tangent));
		// vertex bitangent
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Asset_model::Mesh::Vertex), (void*)offsetof(Asset_model::Mesh::Vertex, m_bitangent));

		glBindVertexArray(0);
	}

	void draw_mesh(const Asset_model::Mesh& in_mesh, const Asset_material& in_material, const glm::mat4& in_mvp)
	{
		// Use our shader
		glUseProgram(in_material.m_program_id);

		GLuint mvp_uniform_loc = glGetUniformLocation(in_material.m_program_id, "MVP");

		if (mvp_uniform_loc == -1)
			return;

		glUniformMatrix4fv(mvp_uniform_loc, 1, GL_FALSE, glm::value_ptr(in_mvp));

		ui32 texture_param_idx = 0;

		for (auto& texture_param : in_material.m_texture_parameters)
		{
			//in future we want to render an error texture instead
			if (!texture_param.m_texture.is_valid())
				continue;

			const i32 uniform_loc = glGetUniformLocation(in_material.m_program_id, texture_param.m_name.c_str());

			//error handle this
			if (uniform_loc == -1)
				continue;

			glActiveTexture(GL_TEXTURE0 + texture_param_idx);
			glBindTexture(GL_TEXTURE_2D, texture_param.m_texture.get()->m_render_id);
			glUniform1i(uniform_loc, texture_param_idx);

			++texture_param_idx;
		}

		// draw mesh
		glBindVertexArray(in_mesh.m_vao);
		glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(in_mesh.m_indices.size()), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

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
				impuls_private::init_texture(in_texture);
			}
	);

	assetsystem_state->do_post_load<Asset_material>
		(
			[](Asset_material& in_material)
			{
				//do opengl load
				impuls_private::init_material(in_material);
			}
	);

	assetsystem_state->do_post_load<Asset_model>
	(
		[](Asset_model& in_model)
		{
			//do opengl load
			for (auto&& mesh : in_model.m_meshes)
				impuls_private::init_mesh(mesh.first);
		}
	);

	assetsystem_state->do_pre_unload<Asset_texture>
		(
			[](Asset_texture& in_texture)
			{
				glDeleteTextures(1, &in_texture.m_render_id);

				if (!in_texture.m_free_texture_data_after_setup)
					in_texture.m_texture_data.reset();
			}
	);

	assetsystem_state->do_pre_unload<Asset_material>
		(
			[](Asset_material& in_material)
			{
				glDeleteProgram(in_material.m_program_id);
			}
	);

	assetsystem_state->do_pre_unload<Asset_model>
		(
			[](Asset_model& in_model)
			{
				for (auto&& mesh : in_model.m_meshes)
				{
					glDeleteVertexArrays(1, &mesh.first.m_vao);
					glDeleteBuffers(1, &mesh.first.m_vbo);
					glDeleteBuffers(1, &mesh.first.m_ebo);
				}
			}
	);

	const State_render_scene* scene_state = in_context.get_state<State_render_scene>();

	if (!scene_state)
		return;

	std::vector<Asset_ref<Asset_texture>> textures_to_load;
	std::vector<Asset_ref<Asset_material>> materials_to_load;
	std::vector<Asset_ref<Asset_model>> models_to_load;

	std::unordered_map<GLFWwindow*, std::vector<const Render_object_view*>> window_to_views_lut;

	for (const Render_object_view& view : scene_state->m_views.m_objects)
	{
		if (!view.m_window_render_on)
			continue;

		window_to_views_lut[view.m_window_render_on].push_back(&view);
	}

	for (auto& window_to_views_it : window_to_views_lut)
	{
		glfwMakeContextCurrent(window_to_views_it.first);

		glClearColor(0.0f, 0.0f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		sic::i32 current_window_x, current_window_y;
		glfwGetWindowSize(window_to_views_it.first, &current_window_x, &current_window_y);

		if (current_window_x == 0 || current_window_y == 0)
			continue;

		const float aspect_ratio = static_cast<float>(current_window_x) / static_cast<float>(current_window_y);

		for (const Render_object_view* view : window_to_views_it.second)
		{
			auto scene_it = scene_state->m_level_id_to_scene_lut.find(view->m_level_id);

			if (scene_it == scene_state->m_level_id_to_scene_lut.end())
				continue;

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

						auto material_override_it = model.m_material_overrides.find(mesh.first.m_material_slot);

						const Asset_ref<Asset_material>& mat_to_draw = material_override_it != model.m_material_overrides.end() ? material_override_it->second : model_asset->get_material(mesh_idx);
						if (mat_to_draw.is_valid())
						{
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
									impuls_private::draw_mesh(mesh.first, *mat_to_draw.get(), mvp);
							}
							else if (mat_to_draw.get_load_state() == Asset_load_state::not_loaded)
							{
								materials_to_load.push_back(mat_to_draw);
							}
						}
					}
				}
				else if (model.m_model.get_load_state() == Asset_load_state::not_loaded)
					models_to_load.push_back(model.m_model);
			}
		}

		glfwSwapBuffers(window_to_views_it.first);
	}

	glfwMakeContextCurrent(nullptr);

	assetsystem_state->load_batch(in_context, std::move(textures_to_load));
	assetsystem_state->load_batch(in_context, std::move(materials_to_load));
	assetsystem_state->load_batch(in_context, std::move(models_to_load));
}
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

namespace sic_private
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

	GLuint create_program(const std::string& in_vertex_shader_path, const std::string& in_fragment_shader_path);

	void init_material(Asset_material& out_material)
	{
		const GLuint program_id = create_program(out_material.m_vertex_shader, out_material.m_fragment_shader);

		if (program_id != 0)
			out_material.m_program_id = program_id;
	}

	GLuint create_program(const std::string& in_vertex_shader_path, const std::string& in_fragment_shader_path)
	{
		// Create the shaders
		GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
		GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

		const std::string vertex_shader_code = File_management::load_file(in_vertex_shader_path, false);

		if (vertex_shader_code.size() == 0)
		{
			SIC_LOG_E(g_log_renderer_verbose, "Impossible to open {0}. Are you in the right directory?", in_vertex_shader_path.c_str());
			return 0;
		}
		const std::string fragment_shader_code = File_management::load_file(in_fragment_shader_path, false);

		if (fragment_shader_code.size() == 0)
		{
			SIC_LOG_E(g_log_renderer_verbose, "Impossible to open {0}. Are you in the right directory?", in_fragment_shader_path.c_str());
			return 0;
		}

		GLint result_id = GL_FALSE;
		i32 info_log_length;

		// Compile Vertex Shader
		SIC_LOG(g_log_renderer_verbose, "Compiling shader : {0}", in_vertex_shader_path.c_str());

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
		SIC_LOG(g_log_renderer_verbose, "Compiling shader : {0}", in_fragment_shader_path.c_str());

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

		return program_id;
	}

	void init_mesh(Asset_model::Mesh& inout_mesh)
	{
		std::vector<Asset_model::Asset_model::Mesh::Vertex>& vertices = inout_mesh.m_vertices;
		std::vector<ui32>& indices = inout_mesh.m_indices;

		glGenVertexArrays(1, &inout_mesh.m_vertexarray);
		glBindVertexArray(inout_mesh.m_vertexarray);

		glGenBuffers(1, &inout_mesh.m_vertexbuffer);
		glBindBuffer(GL_ARRAY_BUFFER, inout_mesh.m_vertexbuffer);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Asset_model::Mesh::Vertex), &vertices[0], GL_STATIC_DRAW);

		glGenBuffers(1, &inout_mesh.m_elementbuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, inout_mesh.m_elementbuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(ui32), &indices[0], GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, inout_mesh.m_vertexbuffer);
		glEnableVertexAttribArray(0);
		// vertex positions
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Asset_model::Mesh::Vertex), (void*)0);

		glEnableVertexAttribArray(1);
		// vertex normals
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Asset_model::Mesh::Vertex), (void*)offsetof(Asset_model::Mesh::Vertex, m_normal));
		
		glEnableVertexAttribArray(2);
		// vertex texture coords
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Asset_model::Mesh::Vertex), (void*)offsetof(Asset_model::Mesh::Vertex, m_tex_coords));
		
		glEnableVertexAttribArray(3);
		// vertex tangent
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Asset_model::Mesh::Vertex), (void*)offsetof(Asset_model::Mesh::Vertex, m_tangent));
		
		glEnableVertexAttribArray(4);
		// vertex bitangent
		glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Asset_model::Mesh::Vertex), (void*)offsetof(Asset_model::Mesh::Vertex, m_bitangent));
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
			{
				SIC_LOG_E(g_log_renderer_verbose,
					"Texture parameter: {0}, not found in material with shaders: {1} and {2}",
					texture_param.m_name.c_str(), in_material.m_vertex_shader.c_str(), in_material.m_fragment_shader.c_str());
				continue;
			}

			glActiveTexture(GL_TEXTURE0 + texture_param_idx);
			glBindTexture(GL_TEXTURE_2D, texture_param.m_texture.get()->m_render_id);
			glUniform1i(uniform_loc, texture_param_idx);

			++texture_param_idx;
		}

		// draw mesh
		glBindVertexArray(in_mesh.m_vertexarray);
		glBindBuffer(GL_ARRAY_BUFFER, in_mesh.m_vertexbuffer);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, in_mesh.m_elementbuffer);
		glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(in_mesh.m_indices.size()), GL_UNSIGNED_INT, 0);

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
				sic_private::init_mesh(mesh.first);
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
					glDeleteVertexArrays(1, &mesh.first.m_vertexarray);
					glDeleteBuffers(1, &mesh.first.m_vertexbuffer);
					glDeleteBuffers(1, &mesh.first.m_elementbuffer);
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

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	for (auto& window_to_views_it : window_to_views_lut)
	{
		glfwMakeContextCurrent(window_to_views_it.first);

		//glBindFramebuffer(GL_FRAMEBUFFER, 0);
		//glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		sic::i32 current_window_x, current_window_y;
		glfwGetWindowSize(window_to_views_it.first, &current_window_x, &current_window_y);

		if (current_window_x == 0 || current_window_y == 0)
			continue;

		const float aspect_ratio = static_cast<float>(current_window_x) / static_cast<float>(current_window_y);

		for (Render_object_view* view : window_to_views_it.second)
		{
			auto scene_it = scene_state->m_level_id_to_scene_lut.find(view->m_level_id);

			if (scene_it == scene_state->m_level_id_to_scene_lut.end())
				continue;

			// Render to our framebuffer
			view->m_render_target.bind_as_target();

			glClearColor(0.0f, 0.0f, 0.1f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glViewport(0, 0, current_window_x * view->m_viewport_size.x, current_window_y * view->m_viewport_size.y);

			// Set "renderedTexture" as our colour attachement #0
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, view->m_render_target.m_texture_id, 0);

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

						auto material_override_it = model.m_material_overrides.find(mesh.first.m_material_slot);

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
								sic_private::draw_mesh(mesh.first, *mat_to_draw.get(), mvp);
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

			static GLuint quad_program_id, tex_loc_id;

			static OpenGl_vertex_buffer_array
				<
				OpenGl_vertex_attribute_float2, //position
				OpenGl_vertex_attribute_float2 //texcoords
				> quad_vertex_buffer_array;

			static OpenGl_buffer quad_indexbuffer(GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW);

			static std::vector<unsigned int> indices;

			if (!one_shot)
			{
				one_shot = true;

				const std::vector<GLfloat> positions = {
					-1.0f * 0.5f, -1.0f * 0.5f,
					-1.0f* 0.5f, 1.0f* 0.5f,
					1.0f * 0.5f, 1.0f* 0.5f,
					1.0f * 0.5f, -1.0f* 0.5f
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

				quad_vertex_buffer_array.set_data<0>(positions);
				quad_vertex_buffer_array.set_data<1>(tex_coords);

				quad_indexbuffer.set_data(indices);

				quad_program_id = sic_private::create_program("content/materials/pass_through.vs", "content/materials/simple_texture.fs");
			}

			
			// Render to the screen
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, current_window_x, current_window_y);

			glUseProgram(quad_program_id);

			tex_loc_id = glGetUniformLocation(quad_program_id, "uniform_texture");
			view->m_render_target.bind_as_texture(0, tex_loc_id);

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
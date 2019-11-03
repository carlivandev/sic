#include "impuls/system_renderer.h"
#include "impuls/transform.h"

#include "impuls/asset_types.h"
#include "impuls/system_window.h"
#include "impuls/view.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "stb/stb_image.h"

namespace impuls_private
{
	using namespace impuls;

	void init_texture(asset_texture& out_texture)
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
		case e_texture_format::gray:
			gl_texture_format = GL_R;
			break;
		case e_texture_format::gray_a:
			gl_texture_format = GL_RG;
			break;
		case e_texture_format::rgb:
			gl_texture_format = GL_RGB;
			break;
		case e_texture_format::rgb_a:
			gl_texture_format = GL_RGBA;
			break;
		default:
			break;
		}

		glTexImage2D(GL_TEXTURE_2D, 0, gl_texture_format, out_texture.m_width, out_texture.m_height, 0, gl_texture_format, GL_UNSIGNED_BYTE, out_texture.m_texture_data);

		glGenerateMipmap(GL_TEXTURE_2D);

		glBindTexture(GL_TEXTURE_2D, 0);

		if (out_texture.m_free_texture_data_after_setup)
		{
			stbi_image_free(out_texture.m_texture_data);
			out_texture.m_texture_data = nullptr;
		}

		//end gpu texture setup
	}

	void init_material(asset_material& out_material)
	{
		const std::string vertex_file_path = out_material.m_vertex_shader;
		const std::string fragment_file_path = out_material.m_fragment_shader;

		// Create the shaders
		GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
		GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

		const std::string vertex_shader_code = file_management::load_file(vertex_file_path, false);

		if (vertex_shader_code.size() == 0)
		{
			printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", vertex_file_path.c_str());
			return;
		}
		const std::string fragment_shader_code = file_management::load_file(fragment_file_path, false);

		if (fragment_shader_code.size() == 0)
		{
			printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", fragment_file_path.c_str());
			return;
		}

		GLint result_id = GL_FALSE;
		i32 info_log_length;

		// Compile Vertex Shader
		printf("Compiling shader : %s\n", vertex_file_path.c_str());
		const GLchar* vertex_source_ptr = vertex_shader_code.data();
		glShaderSource(vertex_shader_id, 1, &vertex_source_ptr, NULL);
		glCompileShader(vertex_shader_id);

		// Check Vertex Shader
		glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &result_id);
		glGetShaderiv(vertex_shader_id, GL_INFO_LOG_LENGTH, &info_log_length);
		if (info_log_length > 0)
		{
			std::vector<char> VertexShaderErrorMessage(static_cast<size_t>(info_log_length) + 1);
			glGetShaderInfoLog(vertex_shader_id, info_log_length, NULL, &VertexShaderErrorMessage[0]);
			printf("%s\n", &VertexShaderErrorMessage[0]);
		}

		// Compile Fragment Shader
		printf("Compiling shader : %s\n", fragment_file_path.c_str());
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
			printf("%s\n", &fragment_shader_error_message[0]);
		}

		// Link the program
		printf("Linking program\n");
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
			printf("%s\n", &program_error_message[0]);
		}

		glDetachShader(program_id, vertex_shader_id);
		glDetachShader(program_id, fragment_shader_id);

		glDeleteShader(vertex_shader_id);
		glDeleteShader(fragment_shader_id);

		out_material.m_program_id = program_id;
	}

	void init_mesh(asset_model::mesh& out_mesh)
	{
		std::vector<asset_model::asset_model::mesh::vertex>& vertices = out_mesh.m_vertices;
		std::vector<ui32>& indices = out_mesh.m_indices;

		glGenVertexArrays(1, &out_mesh.m_vao);
		glGenBuffers(1, &out_mesh.m_vbo);
		glGenBuffers(1, &out_mesh.m_ebo);

		glBindVertexArray(out_mesh.m_vao);
		glBindBuffer(GL_ARRAY_BUFFER, out_mesh.m_vbo);

		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(asset_model::mesh::vertex), &vertices[0], GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, out_mesh.m_ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(ui32), &indices[0], GL_STATIC_DRAW);

		// vertex positions
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(asset_model::mesh::vertex), (void*)0);
		// vertex normals
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(asset_model::mesh::vertex), (void*)offsetof(asset_model::mesh::vertex, m_normal));
		// vertex texture coords
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(asset_model::mesh::vertex), (void*)offsetof(asset_model::mesh::vertex, m_tex_coords));
		// vertex tangent
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(asset_model::mesh::vertex), (void*)offsetof(asset_model::mesh::vertex, m_tangent));
		// vertex bitangent
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(asset_model::mesh::vertex), (void*)offsetof(asset_model::mesh::vertex, m_bitangent));

		glBindVertexArray(0);
	}

	void draw_mesh(const asset_model::mesh& in_mesh, const asset_material& in_material, const glm::mat4& in_mvp)
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

void impuls::system_renderer::on_created(world_context&& in_context) const
{
	in_context.register_state<state_renderer>("renderer_state");
}

void impuls::system_renderer::on_tick(world_context&& in_context, float in_time_delta) const
{
	in_time_delta;

	state_assetsystem* assetsystem_state = in_context.get_state<state_assetsystem>();

	if (!assetsystem_state)
		return;

	state_main_window* mainwindow_state = in_context.get_state<state_main_window>();

	glfwMakeContextCurrent(mainwindow_state->m_window->get<component_window>().m_window);
	
	assetsystem_state->do_post_load<asset_texture>
		(
			[](asset_ref<asset_texture>&& in_texture)
			{
				//do opengl load
				impuls_private::init_texture(*in_texture.get());
			}
	);

	assetsystem_state->do_post_load<asset_material>
		(
			[](asset_ref<asset_material>&& in_material)
			{
				//do opengl load
				impuls_private::init_material(*in_material.get());
			}
	);

	assetsystem_state->do_post_load<asset_model>
	(
		[](asset_ref<asset_model>&& in_model)
		{
			//do opengl load
			for (auto&& mesh : in_model.get()->m_meshes)
				impuls_private::init_mesh(mesh.first);
		}
	);

	state_renderer* renderer_state = in_context.get_state<state_renderer>();

	if (!renderer_state)
		return;

	std::vector<asset_ref<asset_texture>> textures_to_load;
	std::vector<asset_ref<asset_material>> materials_to_load;
	std::vector<asset_ref<asset_model>> models_to_load;

	for (component_view& component_view_it : in_context.each<component_view>())
	{
		if (!component_view_it.m_window_render_on)
			continue;

		component_window& render_window = component_view_it.m_window_render_on->get<component_window>();

		impuls::i32 current_window_x, current_window_y;
		glfwGetWindowSize(render_window.m_window, &current_window_x, &current_window_y);

		if (current_window_x == 0 || current_window_y == 0)
			continue;

		glfwMakeContextCurrent(render_window.m_window);

		// Direction : Spherical coordinates to Cartesian coordinates conversion
		const glm::vec3 direction
		(
			cos(component_view_it.m_vertical_angle) * sin(component_view_it.m_horizontal_angle),
			sin(component_view_it.m_vertical_angle),
			cos(component_view_it.m_vertical_angle) * cos(component_view_it.m_horizontal_angle)
		);

		// Right vector
		const glm::vec3 right = glm::vec3
		(
			sin(component_view_it.m_horizontal_angle - 3.14f / 2.0f),
			0,
			cos(component_view_it.m_horizontal_angle - 3.14f / 2.0f)
		);

		// Up vector : perpendicular to both direction and right
		const glm::vec3 up = glm::cross(right, direction);

		const float fov = 45.0f;
		const float ratio = static_cast<float>(current_window_x) / static_cast<float>(current_window_y);
		const float near_plane = 0.1f;
		const float far_plane = 100.0f;

		const glm::mat4x4 proj_mat = glm::perspective
		(
			glm::radians(fov),
			ratio,
			near_plane,
			far_plane
		);

		// ortho camera :
		//glm::mat4 Projection = glm::ortho(-10.0f,10.0f,-10.0f,10.0f,0.0f,100.0f); // In world coordinates

		const glm::mat4x4 view_mat = glm::lookAt
		(
			component_view_it.m_position,
			component_view_it.m_position + direction,
			up
		);

		glClearColor(0.0f, 0.0f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		renderer_state->m_model_drawcalls.read
		(
			[&textures_to_load, &materials_to_load, &models_to_load, &proj_mat, &view_mat](const std::vector<drawcall_model> & models)
			{
				for (const drawcall_model& model : models)
				{
					if (model.m_model.get_load_state() == e_asset_load_state::loaded)
					{
						asset_model* model_asset = model.m_model.get();

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

							auto material_override_it = model.m_material_overrides.find(mesh.first.material_slot);

							const asset_ref<asset_material> mat_to_draw = material_override_it != model.m_material_overrides.end() ? material_override_it->second : model_asset->get_material(mesh_idx);
							if (mat_to_draw.is_valid())
							{
								if (mat_to_draw.get_load_state() == e_asset_load_state::loaded)
								{
									bool all_texture_loaded = true;

									for (asset_material::texture_parameter& texture_param : mat_to_draw.get()->m_texture_parameters)
									{
										if (texture_param.m_texture.is_valid())
										{
											if (texture_param.m_texture.get_load_state() == e_asset_load_state::not_loaded)
												textures_to_load.push_back(texture_param.m_texture);
											
											if (texture_param.m_texture.get_load_state() != e_asset_load_state::loaded)
												all_texture_loaded = false;
										}
									}

									if (all_texture_loaded)
										impuls_private::draw_mesh(mesh.first, *mat_to_draw.get(), mvp);
								}
								else if (mat_to_draw.get_load_state() == e_asset_load_state::not_loaded)
								{
									materials_to_load.push_back(mat_to_draw);
								}
							}
						}
					}
					else if (model.m_model.get_load_state() == e_asset_load_state::not_loaded)
						models_to_load.push_back(model.m_model);
				}
			}
		);

		glfwSwapBuffers(render_window.m_window);
	}

	glfwMakeContextCurrent(nullptr);

	assetsystem_state->load_batch(in_context, std::move(textures_to_load));
	assetsystem_state->load_batch(in_context, std::move(materials_to_load));
	assetsystem_state->load_batch(in_context, std::move(models_to_load));
}
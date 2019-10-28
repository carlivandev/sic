#include "impuls/system_renderer.h"
#include "impuls/transform.h"

#include "impuls/asset_types.h"
#include "impuls/system_window.h"
#include "impuls/view.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

namespace impuls_private
{
	using namespace impuls;

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
			if (!texture_param.m_texture)
				continue;

			const i32 uniform_loc = glGetUniformLocation(in_material.m_program_id, texture_param.m_name.c_str());

			//error handle this
			if (uniform_loc == -1)
				continue;

			glActiveTexture(GL_TEXTURE0 + texture_param_idx);
			glBindTexture(GL_TEXTURE_2D, texture_param.m_texture->m_render_id);
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
			[&models_to_load, &proj_mat, &view_mat](const std::vector<drawcall_model> & models)
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

							//TODO: get material from drawcall, to get proper material overrides
							if (auto mat_to_draw = model_asset->get_material(mesh_idx))
								impuls_private::draw_mesh(mesh.first, *mat_to_draw, mvp);
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

	assetsystem_state->load_batch(in_context, std::move(models_to_load));
}
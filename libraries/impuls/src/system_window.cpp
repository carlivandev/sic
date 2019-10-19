#include "impuls/system_window.h"

#include "impuls/asset_management.h"
#include "impuls/asset_types.h"
#include "impuls/event.h"
#include "impuls/view.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "impuls/gl_includes.h"

#include "impuls/system_model.h"
#include "impuls/transform.h"

namespace impuls_private
{
	using namespace impuls;

	void glfw_error(int in_error_code, const char* in_message)
	{
		printf("glfw error[%i]: %s\n", in_error_code, in_message);
	}

	void window_resized(GLFWwindow* in_window, i32 in_width, i32 in_height)
	{
		component_window* wdw = static_cast<component_window*>(glfwGetWindowUserPointer(in_window));

		if (!wdw)
			return;

		wdw->m_dimensions_x = in_width;
		wdw->m_dimensions_y = in_height;
	}

	void window_focused(GLFWwindow* in_window, int in_focused)
	{
		component_window* wdw = static_cast<component_window*>(glfwGetWindowUserPointer(in_window));

		if (!wdw)
			return;

		wdw->m_is_focused = in_focused != 0;
	}

	void window_scrolled(GLFWwindow* in_window, double in_xoffset, double in_yoffset)
	{
		component_window* wdw = static_cast<component_window*>(glfwGetWindowUserPointer(in_window));

		if (!wdw)
			return;

		wdw->m_scroll_offset_x += in_xoffset;
		wdw->m_scroll_offset_y += in_yoffset;
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

void impuls::system_window::on_created(world_context&& in_context) const
{
	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		return;
	}

	glfwSetErrorCallback(&impuls_private::glfw_error);

	in_context.register_component_type<component_window>("component_window", 4, 1);
	in_context.register_object<object_window>("window", 4, 1);

	in_context.register_component_type<component_view>("component_view", 4, 1);
	in_context.register_object<object_view>("view", 4, 1);

	in_context.register_state<state_main_window>("state_main_window");

	in_context.listen<impuls::event_created<object_window>>
	(
		[](object_window& new_window)
		{
			glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
			glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL 

			component_window& new_window_component = new_window.get<component_window>();
			new_window_component.m_dimensions_x = 1600;
			new_window_component.m_dimensions_y = 800;

			new_window_component.m_window = glfwCreateWindow(new_window_component.m_dimensions_x, new_window_component.m_dimensions_y, "impuls_test_game", NULL, NULL);

			if (new_window_component.m_window == NULL)
			{
				fprintf(stderr, "Failed to open GLFW window. GPU not 3.3 compatible.\n");
				glfwTerminate();
				return;
			}

			glfwSetWindowUserPointer(new_window_component.m_window, &new_window_component);
			glfwSetWindowSizeCallback(new_window_component.m_window, &impuls_private::window_resized);
			glfwSetWindowFocusCallback(new_window_component.m_window, &impuls_private::window_focused);
			glfwSetScrollCallback(new_window_component.m_window, &impuls_private::window_scrolled);

			glfwMakeContextCurrent(new_window_component.m_window); // Initialize GLEW
			glewExperimental = true; // Needed in core profile
			if (glewInit() != GLEW_OK) {
				fprintf(stderr, "Failed to initialize GLEW\n");
				return;
			}

			// Enable depth test
			glEnable(GL_DEPTH_TEST);
			// Accept fragment if it closer to the camera than the former one
			glDepthFunc(GL_LESS);

			glEnable(GL_CULL_FACE);

			// Ensure we can capture the escape key being pressed below
			glfwSetInputMode(new_window_component.m_window, GLFW_STICKY_KEYS, GL_TRUE);

			glfwMakeContextCurrent(nullptr);
		}
	);
}

void impuls::system_window::on_tick(world_context&& in_context, float in_time_delta) const
{
	in_time_delta;

	state_main_window* main_window_state = in_context.get_state<state_main_window>();

	if (!main_window_state)
		return;

	for (component_view& component_view_it : in_context.each<component_view>())
	{
		if (!component_view_it.m_window_render_on)
			continue;

		component_window& render_window = component_view_it.m_window_render_on->get<component_window>();

		impuls::i32 current_window_x, current_window_y;
		glfwGetWindowSize(render_window.m_window, &current_window_x, &current_window_y);
		
		if (render_window.m_dimensions_x != current_window_x ||
			render_window.m_dimensions_y != current_window_y)
		{
			//handle resize
			glfwSetWindowSize(render_window.m_window, render_window.m_dimensions_x, render_window.m_dimensions_y);
		}

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

		for (auto& model_component : in_context.each<component_model>())
		{
			if (model_component.m_model && model_component.m_transform)
			{
				const glm::mat4 model_mat = glm::mat4(
					1.0f, 0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f, 0.0f,
					model_component.m_transform->m_position.x, model_component.m_transform->m_position.y, model_component.m_transform->m_position.z, 1.0);

				const glm::mat4 mvp = proj_mat * view_mat * model_mat;

				const ui64 mesh_count = model_component.m_model->m_meshes.size();

				for (i32 mesh_idx = 0; mesh_idx < mesh_count; mesh_idx++)
				{
					auto& mesh = model_component.m_model->m_meshes[mesh_idx];

					if (auto mat_to_draw = model_component.get_material(mesh_idx))
						impuls_private::draw_mesh(mesh.first, *mat_to_draw, mvp);
				}
			}
		}

		glfwSwapBuffers(render_window.m_window);
		glfwPollEvents();

		if (glfwWindowShouldClose(render_window.m_window) != 0)
		{
			if (main_window_state->m_window == component_view_it.m_window_render_on)
				in_context.m_world->destroy();
			else
				glfwDestroyWindow(render_window.m_window);
		}
	}

	glfwMakeContextCurrent(nullptr);
}

void impuls::system_window::on_end_simulation(world_context&& in_context) const
{
	for (component_view& component_view_it : in_context.each<component_view>())
	{
		if (!component_view_it.m_window_render_on)
			continue;

		component_window& render_window = component_view_it.m_window_render_on->get<component_window>();
		glfwDestroyWindow(render_window.m_window);
	}

	glfwTerminate();
}

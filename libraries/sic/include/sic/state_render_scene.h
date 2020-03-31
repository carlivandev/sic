#pragma once
#include "sic/engine_context.h"
#include "sic/update_list.h"

#include "sic/gl_includes.h"
#include "sic/asset.h"
#include "sic/opengl_render_target.h"
#include "sic/opengl_draw_interface_debug_lines.h"
#include "sic/renderer_shape_draw_functions.h"
#include "sic/opengl_program.h"
#include "sic/opengl_buffer.h"
#include "sic/opengl_draw_strategies.h"
#include "sic/file_management.h"

#include "glm/mat4x4.hpp"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/ext/quaternion_float.hpp"

#include <optional>
#include <unordered_map>
#include <mutex>

namespace sic
{
	struct Asset_model;
	struct Asset_material;
	struct Object_window;

	struct Render_object_view : Noncopyable
	{
		GLFWwindow* m_window_render_on = nullptr;

		glm::mat4x4 m_view_orientation = glm::mat4x4(1);

		//offset in percentage(0 - 1) based on window size, top-left
		glm::vec2 m_viewport_offset = { 0.0f, 0.0f };
		//size in percentage(0 - 1) based on window size, top-left
		glm::vec2 m_viewport_size = { 1.0f, 1.0f };

		float m_fov = 45.0f;
		float m_near_plane = 0.1f;
		float m_far_plane = 100.0f;

		std::optional<OpenGl_render_target> m_render_target;
		i32 m_level_id = -1;
	};

	struct Render_object_model : Noncopyable
	{
		Asset_ref<Asset_model> m_model;
		std::unordered_map<std::string, Asset_ref<Asset_material>> m_material_overrides;
		glm::mat4x4 m_orientation = glm::mat4x4(1);
	};

	struct Render_object_debug_drawer : Noncopyable
	{
		struct Shape
		{
			virtual void draw(OpenGl_draw_interface_debug_lines& in_out_draw_interface) const = 0;

			glm::vec4 m_color;
			float m_lifetime = 0.0f;
		};

		struct Line : Shape
		{
			void draw(OpenGl_draw_interface_debug_lines& in_out_draw_interface) const final
			{
				sic::renderer_shape_draw_functions::draw_line(in_out_draw_interface, m_start, m_end, m_color);
			}

			glm::vec3 m_start;
			glm::vec3 m_end;
		};

		struct Sphere : Shape
		{
			void draw(OpenGl_draw_interface_debug_lines& in_out_draw_interface) const final
			{
				sic::renderer_shape_draw_functions::draw_sphere(in_out_draw_interface, m_center, m_radius, m_segments, m_color);
			}

			glm::vec3 m_center;
			float m_radius = 0.0f;
			i32 m_segments = 0;
		};

		struct Cube : Shape
		{
			void draw(OpenGl_draw_interface_debug_lines& in_out_draw_interface) const final
			{
				sic::renderer_shape_draw_functions::draw_cube(in_out_draw_interface, m_center, m_half_size, m_rotation, m_color);
			}

			glm::vec3 m_center;
			glm::vec3 m_half_size;
			glm::quat m_rotation;
		};

		struct Cone : Shape
		{
			void draw(OpenGl_draw_interface_debug_lines& in_out_draw_interface) const final
			{
				sic::renderer_shape_draw_functions::draw_cone(in_out_draw_interface, m_origin, m_direction, m_length, m_angle_width, m_angle_height, m_num_sides, m_color);
			}

			glm::vec3 m_origin;
			glm::vec3 m_direction;
			float m_length = 0.0f;
			float m_angle_width = 0.0f;
			float m_angle_height = 0.0f;
			i32 m_num_sides = 0;
		};

		struct Capsule : Shape
		{
			void draw(OpenGl_draw_interface_debug_lines& in_out_draw_interface) const final
			{
				sic::renderer_shape_draw_functions::draw_capsule(in_out_draw_interface, m_center, m_half_height, m_radius, m_rotation, m_color);
			}

			glm::vec3 m_center;
			glm::quat m_rotation;
			float m_half_height = 0.0f;
			float m_radius = 0.0f;
		};

		struct Frustum : Shape
		{
			void draw(OpenGl_draw_interface_debug_lines& in_out_draw_interface) const final
			{
				const glm::mat4x4 frustum_to_world = m_orientation_matrix * glm::inverse(m_projection_matrix);
				sic::renderer_shape_draw_functions::draw_frustum(in_out_draw_interface, frustum_to_world, m_color);
			}

			glm::mat4x4 m_orientation_matrix = glm::mat4x4(1);
			glm::mat4x4 m_projection_matrix = glm::mat4x4(1);
		};

		void draw_shapes(OpenGl_draw_interface_debug_lines& in_out_draw_interface, float in_time_delta)
		{
			std::apply([this, &in_out_draw_interface, in_time_delta](auto&& ... in_args) {(draw_shapes_of_type(in_args, in_out_draw_interface, in_time_delta), ...); }, m_shapes);
		}

		std::tuple
		<
			std::vector<Line>,
			std::vector<Sphere>,
			std::vector<Cube>,
			std::vector<Cone>,
			std::vector<Capsule>,
			std::vector<Frustum>
		> m_shapes;

	private:
		template <typename t_type>
		void draw_shapes_of_type(std::vector<t_type>& in_out_shapes, OpenGl_draw_interface_debug_lines& in_out_draw_interface, float in_time_delta)
		{
			size_t i = 0;
			while (i < in_out_shapes.size())
			{
				t_type& shape = in_out_shapes[i];
				shape.draw(in_out_draw_interface);
				shape.m_lifetime -= in_time_delta;

				if (shape.m_lifetime <= 0.0f)
				{
					if (in_out_shapes.size() > 1)
						shape = in_out_shapes.back();

					in_out_shapes.pop_back();
				}
				else
				{
					++i;
				}
			}
		}
	};

	struct Render_object_window
	{
		const std::string quad_vertex_shader_path = "content/materials/pass_through.vert";
		const std::string quad_fragment_shader_path = "content/materials/simple_texture.frag";

		Render_object_window(GLFWwindow* in_context) :
			m_context(in_context),
			m_quad_program(quad_vertex_shader_path, File_management::load_file(quad_vertex_shader_path), quad_fragment_shader_path, File_management::load_file(quad_fragment_shader_path)),
			m_quad_indexbuffer(GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW)
		{
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
			std::vector<unsigned int> indices = {
				0, 2, 1,
				0, 3, 2
			};

			m_quad_vertex_buffer_array.bind();

			m_quad_vertex_buffer_array.set_data<OpenGl_vertex_attribute_position2D>(positions);
			m_quad_vertex_buffer_array.set_data<OpenGl_vertex_attribute_texcoord>(tex_coords);

			m_quad_indexbuffer.set_data(indices);
		}

		void draw_to_backbuffer()
		{
			sic::i32 current_window_x, current_window_y;
			glfwGetWindowSize(m_context, &current_window_x, &current_window_y);

			if (current_window_x == 0 || current_window_y == 0)
				return;

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			glViewport
			(
				0,
				0,
				static_cast<GLsizei>(current_window_x),
				static_cast<GLsizei>(current_window_y)
			);

			m_quad_program.use();

			GLuint tex_loc_id = m_quad_program.get_uniform_location("uniform_texture");

			m_render_target.value().bind_as_texture(tex_loc_id, 0);

			m_quad_vertex_buffer_array.bind();
			m_quad_indexbuffer.bind();

			OpenGl_draw_strategy_triangle_element::draw(static_cast<GLsizei>(m_quad_indexbuffer.get_max_elements()), 0);
		}

		GLFWwindow* m_context = nullptr;
		std::optional<OpenGl_render_target> m_render_target;
		OpenGl_program m_quad_program;
		OpenGl_vertex_buffer_array
			<
			OpenGl_vertex_attribute_position2D,
			OpenGl_vertex_attribute_texcoord
			> m_quad_vertex_buffer_array;

		OpenGl_buffer m_quad_indexbuffer;
	};

	template <typename t_type>
	struct Render_object_id
	{
		Update_list_id<t_type> m_id;
		i32 m_level_id = -1;
	};

	struct State_render_scene : State
	{
		friend struct System_renderer;
		friend struct System_renderer_state_swapper;

		using Render_scene_level =
			std::tuple
			<
			Update_list<Render_object_view>,
			Update_list<Render_object_model>,
			Update_list<Render_object_debug_drawer>
			>;

		void add_level(i32 in_level_id)
		{
			std::scoped_lock lock(m_update_mutex);
			m_level_id_to_scene_lut[in_level_id];
		}

		void remove_level(i32 in_level_id)
		{
			std::scoped_lock lock(m_update_mutex);
			m_level_id_to_scene_lut.erase(in_level_id);
		}

		template <typename t_type>
		Render_object_id<t_type> create_object(i32 in_level_id, Update_list<t_type>::Update::template Callback&& in_create_callback)
		{
			std::scoped_lock lock(m_update_mutex);

			Update_list<t_type>& list = std::get<Update_list<t_type>>(m_level_id_to_scene_lut.find(in_level_id)->second);
			Render_object_id<t_type> id;
			id.m_id = list.create_object(std::move(in_create_callback));
			id.m_level_id = in_level_id;

			return id;
		}

		template <typename t_type>
		void update_object(Render_object_id<t_type> in_object_id, Update_list<t_type>::Update::template Callback&& in_update_callback)
		{
			std::scoped_lock lock(m_update_mutex);

			Update_list<t_type>& list = std::get<Update_list<t_type>>(m_level_id_to_scene_lut.find(in_object_id.m_level_id)->second);
			list.update_object(in_object_id.m_id, std::move(in_update_callback));
		}

		template <typename t_type>
		void destroy_object(Render_object_id<t_type> in_object_id)
		{
			std::scoped_lock lock(m_update_mutex);

			Update_list<t_type>& list = std::get<Update_list<t_type>>(m_level_id_to_scene_lut.find(in_object_id.m_level_id)->second);
			list.destroy_object(in_object_id.m_id);
		}

	protected:
		void flush_updates();

		std::unordered_map<i32, Render_scene_level> m_level_id_to_scene_lut;
		std::mutex m_update_mutex;
	};
}
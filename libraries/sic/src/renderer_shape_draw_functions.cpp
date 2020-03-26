#include "sic/renderer_shape_draw_functions.h"

#include "sic/opengl_draw_interface_debug_lines.h"
#include "sic/component_transform.h"

namespace sic
{
	struct OpenGl_draw_interface_debug_lines;

	namespace renderer_shape_draw_functions
	{
		void draw_line(OpenGl_draw_interface_debug_lines& in_out_draw_interface, const glm::vec3& in_start, const glm::vec3& in_end, const glm::vec4& in_color)
		{
			in_out_draw_interface.draw_line(in_start, in_end, in_color);
		}

		void draw_cube(OpenGl_draw_interface_debug_lines& in_out_draw_interface, const glm::vec3& in_center, const glm::vec3& in_half_size, const glm::quat& in_rotation, const glm::vec4& in_color)
		{
			glm::vec3 start = in_rotation * (glm::vec3(in_half_size.x, in_half_size.y, in_half_size.z));
			glm::vec3 end = in_rotation * (glm::vec3(in_half_size.x, -in_half_size.y, in_half_size.z));
			in_out_draw_interface.draw_line(in_center + start, in_center + end, in_color);

			start = in_rotation * (glm::vec3(in_half_size.x, -in_half_size.y, in_half_size.z));
			end = in_rotation * (glm::vec3(-in_half_size.x, -in_half_size.y, in_half_size.z));
			in_out_draw_interface.draw_line(in_center + start, in_center + end, in_color);

			start = in_rotation * (glm::vec3(-in_half_size.x, -in_half_size.y, in_half_size.z));
			end = in_rotation * (glm::vec3(-in_half_size.x, in_half_size.y, in_half_size.z));
			in_out_draw_interface.draw_line(in_center + start, in_center + end, in_color);

			start = in_rotation * (glm::vec3(-in_half_size.x, in_half_size.y, in_half_size.z));
			end = in_rotation * (glm::vec3(in_half_size.x, in_half_size.y, in_half_size.z));
			in_out_draw_interface.draw_line(in_center + start, in_center + end, in_color);

			start = in_rotation * (glm::vec3(in_half_size.x, in_half_size.y, -in_half_size.z));
			end = in_rotation * (glm::vec3(in_half_size.x, -in_half_size.y, -in_half_size.z));
			in_out_draw_interface.draw_line(in_center + start, in_center + end, in_color);

			start = in_rotation * (glm::vec3(in_half_size.x, -in_half_size.y, -in_half_size.z));
			end = in_rotation * (glm::vec3(-in_half_size.x, -in_half_size.y, -in_half_size.z));
			in_out_draw_interface.draw_line(in_center + start, in_center + end, in_color);

			start = in_rotation * (glm::vec3(-in_half_size.x, -in_half_size.y, -in_half_size.z));
			end = in_rotation * (glm::vec3(-in_half_size.x, in_half_size.y, -in_half_size.z));
			in_out_draw_interface.draw_line(in_center + start, in_center + end, in_color);

			start = in_rotation * (glm::vec3(-in_half_size.x, in_half_size.y, -in_half_size.z));
			end = in_rotation * (glm::vec3(in_half_size.x, in_half_size.y, -in_half_size.z));
			in_out_draw_interface.draw_line(in_center + start, in_center + end, in_color);

			start = in_rotation * (glm::vec3(in_half_size.x, in_half_size.y, in_half_size.z));
			end = in_rotation * (glm::vec3(in_half_size.x, in_half_size.y, -in_half_size.z));
			in_out_draw_interface.draw_line(in_center + start, in_center + end, in_color);

			start = in_rotation * (glm::vec3(in_half_size.x, -in_half_size.y, in_half_size.z));
			end = in_rotation * (glm::vec3(in_half_size.x, -in_half_size.y, -in_half_size.z));
			in_out_draw_interface.draw_line(in_center + start, in_center + end, in_color);

			start = in_rotation * (glm::vec3(-in_half_size.x, -in_half_size.y, in_half_size.z));
			end = in_rotation * (glm::vec3(-in_half_size.x, -in_half_size.y, -in_half_size.z));
			in_out_draw_interface.draw_line(in_center + start, in_center + end, in_color);

			start = in_rotation * (glm::vec3(-in_half_size.x, in_half_size.y, in_half_size.z));
			end = in_rotation * (glm::vec3(-in_half_size.x, in_half_size.y, -in_half_size.z));
			in_out_draw_interface.draw_line(in_center + start, in_center + end, in_color);
		}

		void draw_sphere(OpenGl_draw_interface_debug_lines& in_out_draw_interface, const glm::vec3& in_center, float in_radius, i32 in_segments, const glm::vec4& in_color)
		{
			in_segments = (glm::max)(in_segments, 4);

			glm::vec3 vertex_0, vertex_1, vertex_2, vertex_3;
			const float angle_inc = 2.f * sic::constants::pi / float(in_segments);
			i32 num_segments_y = in_segments;
			float latitude = angle_inc;
			i32 num_segments_x;
			float longitude;
			float sin_y0 = 0.0f, cos_y0 = 1.0f, sin_y1, cos_y1;
			float sin_x, cos_x;

			while (num_segments_y--)
			{
				sin_y1 = glm::sin(latitude);
				cos_y1 = glm::cos(latitude);

				vertex_0 = glm::vec3(sin_y0, cos_y0, 0.0f) * in_radius + in_center;
				vertex_2 = glm::vec3(sin_y1, cos_y1, 0.0f) * in_radius + in_center;
				longitude = angle_inc;

				num_segments_x = in_segments;
				while (num_segments_x--)
				{
					sin_x = glm::sin(longitude);
					cos_x = glm::cos(longitude);

					vertex_1 = glm::vec3((cos_x * sin_y0), cos_y0, (sin_x * sin_y0)) * in_radius + in_center;
					vertex_3 = glm::vec3((cos_x * sin_y1), cos_y1, (sin_x * sin_y1)) * in_radius + in_center;

					in_out_draw_interface.draw_line(vertex_0, vertex_1, in_color);
					in_out_draw_interface.draw_line(vertex_0, vertex_2, in_color);

					vertex_0 = vertex_1;
					vertex_2 = vertex_3;
					longitude += angle_inc;
				}
				sin_y0 = sin_y1;
				cos_y0 = cos_y1;
				latitude += angle_inc;
			}
		}

		//angles are in radians
		void draw_cone(OpenGl_draw_interface_debug_lines& in_out_draw_interface, const glm::vec3& in_origin, const glm::vec3& in_direction, float in_length, float in_angle_width, float in_angle_height, i32 in_num_sides, const glm::vec4& in_color)
		{
			in_num_sides = (glm::max)(in_num_sides, 4);

			const float angle0 = glm::clamp(in_angle_height, 0.001f, (float)(sic::constants::pi - 0.001f));
			const float angle1 = glm::clamp(in_angle_width, 0.001f, (float)(sic::constants::pi - 0.001f));

			const float sin_x_2 = glm::sin(0.5f * angle0);
			const float sin_y_2 = glm::sin(0.5f * angle1);

			const float sin_sq_x_2 = sin_x_2 * sin_x_2;
			const float sin_sq_y_2 = sin_y_2 * sin_y_2;

			const float tan_x_2 = glm::tan(0.5f * angle0);
			const float tan_y_2 = glm::tan(0.5f * angle1);

			std::vector<glm::vec3> cone_vertices;
			cone_vertices.resize(in_num_sides);

			for (i32 i = 0; i < in_num_sides; i++)
			{
				const float fraction = (float)i / (float)(in_num_sides);
				const float thi = 2.f * sic::constants::pi * fraction;
				const float phi = std::atan2(glm::sin(thi) * sin_y_2, glm::cos(thi) * sin_x_2);
				const float sin_phi = glm::sin(phi);
				const float cos_phi = glm::cos(phi);
				const float sin_sq_phi = sin_phi * sin_phi;
				const float cos_sq_phi = cos_phi * cos_phi;

				const float r_sq = sin_sq_x_2 * sin_sq_y_2 / (sin_sq_x_2 * sin_sq_phi + sin_sq_y_2 * cos_sq_phi);
				const float r = glm::sqrt(r_sq);
				const float sqr = glm::sqrt(1 - r_sq);
				const float alpha = r * cos_phi;
				const float beta = r * sin_phi;

				cone_vertices[i].x = 2 * sqr * alpha;
				cone_vertices[i].y = 2 * sqr * beta;
				cone_vertices[i].z = -(1 - 2 * r_sq);
			}

			sic::Transform transform;
			transform.set_translation(in_origin);
			transform.look_at(in_direction, sic::Transform::up);

			const glm::mat4x4 cone_to_world = glm::scale(transform.get_matrix(), glm::vec3(in_length));

			glm::vec3 current_point, previous_point, first_point;
			for (i32 i = 0; i < in_num_sides; i++)
			{
				current_point = cone_to_world * glm::vec4(cone_vertices[i], 1.0f);
				in_out_draw_interface.draw_line(in_origin, current_point, in_color);

				// previous_point must be defined to draw junctions
				if (i > 0)
				{
					in_out_draw_interface.draw_line(previous_point, current_point, in_color);
				}
				else
				{
					first_point = current_point;
				}

				previous_point = current_point;
			}

			in_out_draw_interface.draw_line(current_point, first_point, in_color);
		}

		void draw_half_circle(OpenGl_draw_interface_debug_lines& in_out_draw_interface, const glm::vec3& in_base, const glm::vec3& in_z_axis, const glm::vec3& in_x_axis, const glm::vec4& in_color, float in_radius, i32 in_num_sides)
		{
			float angle_delta = 2.0f * (float)sic::constants::pi / ((float)in_num_sides);
			glm::vec3 last_vertex = in_base + in_z_axis * in_radius;

			for (i32 side_index = 0; side_index < (in_num_sides / 2); side_index++)
			{
				const glm::vec3 vertex = in_base + (in_z_axis * glm::cos(angle_delta * (side_index + 1)) + in_x_axis * glm::sin(angle_delta * (side_index + 1))) * in_radius;
				in_out_draw_interface.draw_line(last_vertex, vertex, in_color);
				last_vertex = vertex;
			}
		}

		void draw_circle(OpenGl_draw_interface_debug_lines& in_out_draw_interface, const glm::vec3& in_base, const glm::vec3& in_z_axis, const glm::vec3& in_x_axis, const glm::vec4& in_color, float in_radius, i32 in_num_sides)
		{
			const float	angle_delta = 2.0f * sic::constants::pi / in_num_sides;
			glm::vec3 last_vertex = in_base + in_z_axis * in_radius;

			for (i32 side_index = 0; side_index < in_num_sides; side_index++)
			{
				const glm::vec3 vertex = in_base + (in_z_axis * glm::cos(angle_delta * (side_index + 1)) + in_x_axis * glm::sin(angle_delta * (side_index + 1))) * in_radius;
				in_out_draw_interface.draw_line(last_vertex, vertex, in_color);
				last_vertex = vertex;
			}
		}

		void draw_capsule(OpenGl_draw_interface_debug_lines& in_out_draw_interface, const glm::vec3& in_center, float in_half_height, float in_radius, const glm::quat& in_rotation, const glm::vec4& in_color)
		{
			const i32 draw_collision_sides = 32;

			Transform transform;
			transform.set_rotation(in_rotation);

			const glm::vec3 x_axis = transform.get_right();
			const glm::vec3 y_axis = transform.get_up();
			const glm::vec3 z_axis = transform.get_forward();

			// Draw top and bottom circles
			const float half_axis = (glm::max)(in_half_height - in_radius, 1.f);
			const glm::vec3 top_end = in_center + half_axis * y_axis;
			const glm::vec3 bottom_end = in_center - half_axis * y_axis;

			draw_circle(in_out_draw_interface, top_end, z_axis, x_axis, in_color, in_radius, draw_collision_sides);
			draw_circle(in_out_draw_interface, bottom_end, z_axis, x_axis, in_color, in_radius, draw_collision_sides);

			// Draw domed caps
			draw_half_circle(in_out_draw_interface, top_end, x_axis, y_axis, in_color, in_radius, draw_collision_sides);
			draw_half_circle(in_out_draw_interface, top_end, z_axis, y_axis, in_color, in_radius, draw_collision_sides);

			const glm::vec3 negative_y_axis = -y_axis;

			draw_half_circle(in_out_draw_interface, bottom_end, x_axis, negative_y_axis, in_color, in_radius, draw_collision_sides);
			draw_half_circle(in_out_draw_interface, bottom_end, z_axis, negative_y_axis, in_color, in_radius, draw_collision_sides);

			// Draw connected lines
			in_out_draw_interface.draw_line(top_end + in_radius * z_axis, bottom_end + in_radius * z_axis, in_color);
			in_out_draw_interface.draw_line(top_end - in_radius * z_axis, bottom_end - in_radius * z_axis, in_color);
			in_out_draw_interface.draw_line(top_end + in_radius * x_axis, bottom_end + in_radius * x_axis, in_color);
			in_out_draw_interface.draw_line(top_end - in_radius * x_axis, bottom_end - in_radius * x_axis, in_color);
		}

		void draw_frustum(OpenGl_draw_interface_debug_lines& in_out_draw_interface, const glm::mat4x4& in_frustum_to_world, const glm::vec4& in_color)
		{
			glm::vec3 vertices[2][2][2];
			for (ui32 z = 0; z < 2; z++)
			{
				for (ui32 y = 0; y < 2; y++)
				{
					for (ui32 x = 0; x < 2; x++)
					{
						const glm::vec4 unprojected_vertex = in_frustum_to_world *
							(
								glm::vec4
								(
								(x ? -1.0f : 1.0f),
									(y ? -1.0f : 1.0f),
									(z ? 0.0f : 1.0f),
									1.0f
								)
								);
						vertices[x][y][z] = glm::vec3(unprojected_vertex) / unprojected_vertex.w;
					}
				}
			}

			in_out_draw_interface.draw_line(vertices[0][0][0], vertices[0][0][1], in_color);
			in_out_draw_interface.draw_line(vertices[1][0][0], vertices[1][0][1], in_color);
			in_out_draw_interface.draw_line(vertices[0][1][0], vertices[0][1][1], in_color);
			in_out_draw_interface.draw_line(vertices[1][1][0], vertices[1][1][1], in_color);

			in_out_draw_interface.draw_line(vertices[0][0][0], vertices[0][1][0], in_color);
			in_out_draw_interface.draw_line(vertices[1][0][0], vertices[1][1][0], in_color);
			in_out_draw_interface.draw_line(vertices[0][0][1], vertices[0][1][1], in_color);
			in_out_draw_interface.draw_line(vertices[1][0][1], vertices[1][1][1], in_color);

			in_out_draw_interface.draw_line(vertices[0][0][0], vertices[1][0][0], in_color);
			in_out_draw_interface.draw_line(vertices[0][1][0], vertices[1][1][0], in_color);
			in_out_draw_interface.draw_line(vertices[0][0][1], vertices[1][0][1], in_color);
			in_out_draw_interface.draw_line(vertices[0][1][1], vertices[1][1][1], in_color);
		}
	}
}

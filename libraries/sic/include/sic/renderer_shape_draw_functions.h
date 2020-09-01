#pragma once
#include "sic/core/defines.h"

#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/ext/quaternion_float.hpp"

namespace sic
{
	struct OpenGl_draw_interface_debug_lines;

	namespace renderer_shape_draw_functions
	{
		void draw_line(OpenGl_draw_interface_debug_lines& in_out_draw_interface, const glm::vec3& in_start, const glm::vec3& in_end, const glm::vec4& in_color);

		void draw_cube(OpenGl_draw_interface_debug_lines& in_out_draw_interface, const glm::vec3& in_center, const glm::vec3& in_half_size, const glm::quat& in_rotation, const glm::vec4& in_color);

		void draw_sphere(OpenGl_draw_interface_debug_lines& in_out_draw_interface, const glm::vec3& in_center, float in_radius, i32 in_segments, const glm::vec4& in_color);

		//angles are in radians
		void draw_cone(OpenGl_draw_interface_debug_lines& in_out_draw_interface, const glm::vec3& in_origin, const glm::vec3& in_direction, float in_length, float in_angle_width, float in_angle_height, i32 in_num_sides, const glm::vec4& in_color);

		void draw_half_circle(OpenGl_draw_interface_debug_lines& in_out_draw_interface, const glm::vec3& in_base, const glm::vec3& in_z_axis, const glm::vec3& in_x_axis, const glm::vec4& in_color, float in_radius, i32 in_num_sides);

		void draw_circle(OpenGl_draw_interface_debug_lines& in_out_draw_interface, const glm::vec3& in_base, const glm::vec3& in_z_axis, const glm::vec3& in_x_axis, const glm::vec4& in_color, float in_radius, i32 in_num_sides);

		void draw_capsule(OpenGl_draw_interface_debug_lines& in_out_draw_interface, const glm::vec3& in_center, float in_half_height, float in_radius, const glm::quat& in_rotation, const glm::vec4& in_color);

		void draw_frustum(OpenGl_draw_interface_debug_lines& in_out_draw_interface, const glm::mat4x4& in_frustum_to_world, const glm::vec4& in_color);
	}
}
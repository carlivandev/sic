#pragma once
#include "sic/core/defines.h"
#include "sic/core/level_context.h"

#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/ext/quaternion_float.hpp"

namespace sic
{
	namespace debug_draw
	{
		void line(Scene_context& inout_context, const glm::vec3& in_start, const glm::vec3& in_end, const glm::vec4& in_color, float in_lifetime = -1);

		void cube(Scene_context& inout_context, const glm::vec3& in_center, const glm::vec3& in_half_size, const glm::quat& in_rotation, const glm::vec4& in_color, float in_lifetime = -1);

		void sphere(Scene_context& inout_context, const glm::vec3& in_center, float in_radius, i32 in_segments, const glm::vec4& in_color, float in_lifetime = -1);

		//angles are in radians
		void cone(Scene_context& inout_context, const glm::vec3& in_origin, const glm::vec3& in_direction, float in_length, float in_angle_width, float in_angle_height, i32 in_num_sides, const glm::vec4& in_color, float in_lifetime = -1);

		void capsule(Scene_context& inout_context, const glm::vec3& in_center, float in_half_height, float in_radius, const glm::quat& in_rotation, const glm::vec4& in_color, float in_lifetime = -1);

		void frustum(Scene_context& inout_context, const glm::mat4x4& in_orientation_matrix, const glm::mat4x4& in_projection_matrix, const glm::vec4& in_color, float in_lifetime = -1);
	}
}
#include "sic/debug_draw_helpers.h"
#include "sic/system_renderer.h"

namespace sic
{
	namespace debug_draw
	{
		namespace private_functions
		{
			template <typename t_type>
			void draw_shape(Level_context& inout_context, const t_type& in_shape)
			{
				Engine_context engine_context = inout_context.get_engine_context();

				State_debug_drawing* debug_drawing_state = engine_context.get_state<State_debug_drawing>();
				State_render_scene* render_scene_state = engine_context.get_state<State_render_scene>();

				render_scene_state->update_object
				(
					debug_drawing_state->m_level_id_to_debug_drawer_ids.find(inout_context.get_outermost_level_id())->second,
					[in_shape](Render_object_debug_drawer& inout_drawer)
					{
						std::get<std::vector<t_type>>(inout_drawer.m_shapes).push_back(in_shape);
					}
				);
			}
		}

		void line(Level_context& inout_context, const glm::vec3& in_start, const glm::vec3& in_end, const glm::vec4& in_color, float in_lifetime)
		{
			Render_object_debug_drawer::Line shape;
			shape.m_start = in_start;
			shape.m_end = in_end;
			shape.m_color = in_color;
			shape.m_lifetime = in_lifetime;

			private_functions::draw_shape(inout_context, shape);
		}

		void cube(Level_context& inout_context, const glm::vec3& in_center, const glm::vec3& in_half_size, const glm::quat& in_rotation, const glm::vec4& in_color, float in_lifetime)
		{
			Render_object_debug_drawer::Cube shape;
			shape.m_center = in_center;
			shape.m_half_size = in_half_size;
			shape.m_rotation = in_rotation;
			shape.m_color = in_color;
			shape.m_lifetime = in_lifetime;

			private_functions::draw_shape(inout_context, shape);
		}

		void sphere(Level_context& inout_context, const glm::vec3& in_center, float in_radius, i32 in_segments, const glm::vec4& in_color, float in_lifetime)
		{
			Render_object_debug_drawer::Sphere shape;
			shape.m_center = in_center;
			shape.m_radius = in_radius;
			shape.m_segments = in_segments;
			shape.m_color = in_color;
			shape.m_lifetime = in_lifetime;

			private_functions::draw_shape(inout_context, shape);
		}

		//angles are in radians
		void cone(Level_context& inout_context, const glm::vec3& in_origin, const glm::vec3& in_direction, float in_length, float in_angle_width, float in_angle_height, i32 in_num_sides, const glm::vec4& in_color, float in_lifetime)
		{
			Render_object_debug_drawer::Cone shape;
			shape.m_origin = in_origin;
			shape.m_direction = in_direction;
			shape.m_length = in_length;
			shape.m_angle_width = in_angle_width;
			shape.m_angle_height = in_angle_height;
			shape.m_num_sides = in_num_sides;
			shape.m_color = in_color;
			shape.m_lifetime = in_lifetime;

			private_functions::draw_shape(inout_context, shape);
		}

		void capsule(Level_context& inout_context, const glm::vec3& in_center, float in_half_height, float in_radius, const glm::quat& in_rotation, const glm::vec4& in_color, float in_lifetime)
		{
			Render_object_debug_drawer::Capsule shape;
			shape.m_center = in_center;
			shape.m_half_height = in_half_height;
			shape.m_radius = in_radius;
			shape.m_rotation = in_rotation;
			shape.m_color = in_color;
			shape.m_lifetime = in_lifetime;

			private_functions::draw_shape(inout_context, shape);
		}

		void frustum(Level_context& inout_context, const glm::mat4x4& in_orientation_matrix, const glm::mat4x4& in_projection_matrix, const glm::vec4& in_color, float in_lifetime)
		{
			Render_object_debug_drawer::Frustum shape;
			shape.m_orientation_matrix = in_orientation_matrix;
			shape.m_projection_matrix = in_projection_matrix;
			shape.m_color = in_color;
			shape.m_lifetime = in_lifetime;

			private_functions::draw_shape(inout_context, shape);
		}
	}
}

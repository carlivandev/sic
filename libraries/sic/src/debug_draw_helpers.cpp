#include "sic/debug_draw_helpers.h"

#include "sic/state_render_scene.h"

namespace sic
{
	namespace debug_draw
	{
		namespace private_functions
		{
			template <typename T_type>
			void draw_shape(Processor_debug_draw in_processor, const T_type& in_shape)
			{
				const State_debug_drawing& debug_drawing_state = in_processor.get_state_checked_r<State_debug_drawing>();
				in_processor.update_state_deferred<State_render_scene>
				(
					[
						obj_id = debug_drawing_state.m_level_id_to_debug_drawer_ids.find(Scene_context(*in_processor.m_engine, *in_processor.m_scene).get_outermost_scene_id())->second,
						in_shape
					]
					(State_render_scene& inout_render_scene_state)
					{
						 inout_render_scene_state.update_object
						(
							obj_id,
							[in_shape](Render_object_debug_drawer& inout_drawer)
							{
								std::get<std::vector<T_type>>(inout_drawer.m_shapes).push_back(in_shape);
							}
						);
					}
				);
			}
		}

		void line(Processor_debug_draw in_processor, const glm::vec3& in_start, const glm::vec3& in_end, const glm::vec4& in_color, float in_lifetime)
		{
			Render_object_debug_drawer::Line shape;
			shape.m_start = in_start;
			shape.m_end = in_end;
			shape.m_color = in_color;
			shape.m_lifetime = in_lifetime;

			private_functions::draw_shape(in_processor, shape);
		}

		void cube(Processor_debug_draw in_processor, const glm::vec3& in_center, const glm::vec3& in_half_size, const glm::quat& in_rotation, const glm::vec4& in_color, float in_lifetime)
		{
			Render_object_debug_drawer::Cube shape;
			shape.m_center = in_center;
			shape.m_half_size = in_half_size;
			shape.m_rotation = in_rotation;
			shape.m_color = in_color;
			shape.m_lifetime = in_lifetime;

			private_functions::draw_shape(in_processor, shape);
		}

		void sphere(Processor_debug_draw in_processor, const glm::vec3& in_center, float in_radius, i32 in_segments, const glm::vec4& in_color, float in_lifetime)
		{
			Render_object_debug_drawer::Sphere shape;
			shape.m_center = in_center;
			shape.m_radius = in_radius;
			shape.m_segments = in_segments;
			shape.m_color = in_color;
			shape.m_lifetime = in_lifetime;

			private_functions::draw_shape(in_processor, shape);
		}

		//angles are in radians
		void cone(Processor_debug_draw in_processor, const glm::vec3& in_origin, const glm::vec3& in_direction, float in_length, float in_angle_width, float in_angle_height, i32 in_num_sides, const glm::vec4& in_color, float in_lifetime)
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

			private_functions::draw_shape(in_processor, shape);
		}

		void capsule(Processor_debug_draw in_processor, const glm::vec3& in_center, float in_half_height, float in_radius, const glm::quat& in_rotation, const glm::vec4& in_color, float in_lifetime)
		{
			Render_object_debug_drawer::Capsule shape;
			shape.m_center = in_center;
			shape.m_half_height = in_half_height;
			shape.m_radius = in_radius;
			shape.m_rotation = in_rotation;
			shape.m_color = in_color;
			shape.m_lifetime = in_lifetime;

			private_functions::draw_shape(in_processor, shape);
		}

		void frustum(Processor_debug_draw in_processor, const glm::mat4x4& in_orientation_matrix, const glm::mat4x4& in_projection_matrix, const glm::vec4& in_color, float in_lifetime)
		{
			Render_object_debug_drawer::Frustum shape;
			shape.m_orientation_matrix = in_orientation_matrix;
			shape.m_projection_matrix = in_projection_matrix;
			shape.m_color = in_color;
			shape.m_lifetime = in_lifetime;

			private_functions::draw_shape(in_processor, shape);
		}
	}
}

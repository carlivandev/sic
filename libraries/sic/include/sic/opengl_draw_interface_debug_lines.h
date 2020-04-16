#pragma once
#include "sic/defines.h"
#include "sic/type_restrictions.h"
#include "sic/opengl_program.h"

#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

#include <vector>

namespace sic
{
	struct OpenGl_uniform_block_view;

	struct OpenGl_draw_interface_debug_lines : Noncopyable
	{
		static inline const char* simple_line_vertex_shader_path = "content/materials/simple_line.vert";
		static inline const char* simple_line_fragment_shader_path = "content/materials/simple_line.frag";

		constexpr static ui32 max_lines_per_batch = 10000;

		OpenGl_draw_interface_debug_lines(const OpenGl_uniform_block_view& in_uniform_block_view);

		void begin_frame();
		void draw_line(const glm::vec3& in_start, const glm::vec3& in_end, const glm::vec4& in_color);
		void end_frame();

		void flush();

		OpenGl_program simple_line_program;
		OpenGl_vertex_buffer_array<OpenGl_vertex_attribute_position3D, OpenGl_vertex_attribute_color> simple_line_vertex_buffer_array;

		std::vector<glm::vec3> m_line_points;
		std::vector<glm::vec4> m_line_colors;
		glm::vec3* m_line_point_current = nullptr;
		glm::vec4* m_line_color_current = nullptr;
	};
}
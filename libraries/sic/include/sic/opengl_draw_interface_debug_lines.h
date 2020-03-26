#pragma once
#include "sic/defines.h"
#include "sic/type_restrictions.h"

#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

#include <vector>

namespace sic
{
	struct OpenGl_draw_interface_debug_lines : Noncopyable
	{
		constexpr static ui32 max_lines_per_batch = 10000;

		OpenGl_draw_interface_debug_lines();

		void begin_frame();
		void draw_line(const glm::vec3& in_start, const glm::vec3& in_end, const glm::vec4& in_color);
		void end_frame();

		void flush();

		std::vector<glm::vec3> m_line_points;
		std::vector<glm::vec4> m_line_colors;
		glm::vec3* m_line_point_current = nullptr;
		glm::vec4* m_line_color_current = nullptr;
	};
}
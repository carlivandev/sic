#pragma once
#include "sic/defines.h"
#include "sic/type_restrictions.h"
#include "sic/gl_includes.h"

namespace sic
{
	template <GLenum t_mode>
	struct OpenGl_draw_strategy_array
	{
		static void draw(GLint in_first, GLsizei in_count)
		{
			SIC_GL_CHECK(glDrawArrays(t_mode, in_first, in_count));
		}
	};

	template <GLenum t_mode, GLenum t_type>
	struct OpenGl_draw_strategy_element
	{
		static void draw(GLsizei in_count, const void* in_indices)
		{
			SIC_GL_CHECK(glDrawElements(t_mode, in_count, t_type, in_indices));
		}
	};

	struct OpenGl_draw_strategy_line_array : OpenGl_draw_strategy_array<GL_LINES> {};

	struct OpenGl_draw_strategy_triangle_element : OpenGl_draw_strategy_element<GL_TRIANGLES, GL_UNSIGNED_INT> {};
}
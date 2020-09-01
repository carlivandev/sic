#pragma once
#include "sic/gl_includes.h"

#include "sic/core/defines.h"
#include "sic/core/type_restrictions.h"

namespace sic
{
	template <GLenum T_mode>
	struct OpenGl_draw_strategy_array
	{
		static void draw(GLint in_first, GLsizei in_count)
		{
			SIC_GL_CHECK(glDrawArrays(T_mode, in_first, in_count));
		}
	};

	template <GLenum T_mode, GLenum T_type>
	struct OpenGl_draw_strategy_element
	{
		static void draw(GLsizei in_count, const void* in_indices)
		{
			SIC_GL_CHECK(glDrawElements(T_mode, in_count, T_type, in_indices));
		}
	};

	template <GLenum T_mode, GLenum T_type>
	struct OpenGl_draw_strategy_element_instanced
	{
		static void draw(GLsizei in_indices_count, const void* in_indices, GLsizei in_instance_count)
		{
			SIC_GL_CHECK(glDrawElementsInstanced(T_mode, in_indices_count, T_type, in_indices, in_instance_count));
		}
	};

	struct OpenGl_draw_strategy_line_array : OpenGl_draw_strategy_array<GL_LINES> {};

	struct OpenGl_draw_strategy_triangle_element : OpenGl_draw_strategy_element<GL_TRIANGLES, GL_UNSIGNED_INT> {};

	struct OpenGl_draw_strategy_triangle_element_instanced : OpenGl_draw_strategy_element_instanced<GL_TRIANGLES, GL_UNSIGNED_INT> {};
}
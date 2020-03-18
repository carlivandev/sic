#pragma once
#include "sic/defines.h"
#include "sic/gl_includes.h"

namespace sic
{
	struct OpenGl_vertex_attribute_base
	{
		OpenGl_vertex_attribute_base() : m_buffer(GL_ARRAY_BUFFER, GL_STATIC_DRAW) {}
		OpenGl_buffer m_buffer;
	};

	template <GLenum t_type, ui32 t_size>
	struct OpenGl_vertex_attribute : OpenGl_vertex_attribute_base
	{
		static constexpr GLenum get_type() { return t_type; }
		static constexpr ui32 get_size() { return t_size; }
	};

	struct OpenGl_vertex_attribute_position3D : OpenGl_vertex_attribute<GL_FLOAT, 3> {};
	struct OpenGl_vertex_attribute_position2D : OpenGl_vertex_attribute<GL_FLOAT, 2> {};
	struct OpenGl_vertex_attribute_normal : OpenGl_vertex_attribute<GL_FLOAT, 3> {};
	struct OpenGl_vertex_attribute_texcoord : OpenGl_vertex_attribute<GL_FLOAT, 2> {};
	struct OpenGl_vertex_attribute_tangent : OpenGl_vertex_attribute<GL_FLOAT, 3> {};
	struct OpenGl_vertex_attribute_bitangent : OpenGl_vertex_attribute<GL_FLOAT, 3> {};
}
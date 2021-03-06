#pragma once
#include "sic/gl_includes.h"

#include "sic/core/defines.h"
#include "sic/core/type_restrictions.h"

#include <string_view>

namespace sic
{
	struct OpenGl_vertex_attribute_base : Noncopyable
	{
		OpenGl_vertex_attribute_base() : m_buffer(OpenGl_buffer::Creation_params{ OpenGl_buffer_target::array, OpenGl_buffer_usage::static_draw }) {}
		OpenGl_buffer m_buffer;
	};

	template <GLenum T_data_type, ui32 t_size, const char* T_name, GLenum t_shader_data_type>
	struct OpenGl_vertex_attribute : OpenGl_vertex_attribute_base
	{
		static constexpr GLenum get_data_type() { return T_data_type; }
		static constexpr ui32 get_size() { return t_size; }
		static constexpr const char* get_name() { return T_name; }
		static constexpr GLenum get_shader_data_type() { return t_shader_data_type; }
	};

	namespace vertex_attribute_names
	{
		constexpr char position3D[] = "in_position3D";
		constexpr char position2D[] = "in_position2D";
		constexpr char normal[]		= "in_normal";
		constexpr char texcoord[]	= "in_texcoord";
		constexpr char tangent[]	= "in_tangent";
		constexpr char bitangent[]	= "in_bitangent";
		constexpr char color[] = "in_color";
	}

	struct OpenGl_vertex_attribute_position3D : OpenGl_vertex_attribute<GL_FLOAT, 3, vertex_attribute_names::position3D, GL_FLOAT_VEC3> {};
	struct OpenGl_vertex_attribute_position2D : OpenGl_vertex_attribute<GL_FLOAT, 2, vertex_attribute_names::position2D, GL_FLOAT_VEC2> {};
	struct OpenGl_vertex_attribute_normal : OpenGl_vertex_attribute<GL_FLOAT, 3, vertex_attribute_names::normal, GL_FLOAT_VEC3> {};
	struct OpenGl_vertex_attribute_texcoord : OpenGl_vertex_attribute<GL_FLOAT, 2, vertex_attribute_names::texcoord, GL_FLOAT_VEC2> {};
	struct OpenGl_vertex_attribute_tangent : OpenGl_vertex_attribute<GL_FLOAT, 3, vertex_attribute_names::tangent, GL_FLOAT_VEC3> {};
	struct OpenGl_vertex_attribute_bitangent : OpenGl_vertex_attribute<GL_FLOAT, 3, vertex_attribute_names::bitangent, GL_FLOAT_VEC3> {};
	struct OpenGl_vertex_attribute_color : OpenGl_vertex_attribute<GL_FLOAT, 4, vertex_attribute_names::color, GL_FLOAT_VEC4> {};
}
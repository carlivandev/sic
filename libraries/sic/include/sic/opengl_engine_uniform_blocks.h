#pragma once
#include "sic/opengl_uniform_block.h"

namespace sic
{	
	namespace uniform_block_names
	{
		constexpr char block_view[] = "block_view";
		constexpr char block_test[] = "block_test";
		constexpr char block_lights[] = "block_lights";
	}

	struct OpenGl_uniform_block_view : OpenGl_static_uniform_block<OpenGl_uniform_block_view, uniform_block_names::block_view, GL_DYNAMIC_DRAW, glm::mat4x4, glm::mat4x4, glm::mat4x4> {};
	struct OpenGl_uniform_block_test : OpenGl_static_uniform_block<OpenGl_uniform_block_test, uniform_block_names::block_test, GL_STATIC_DRAW, glm::vec3, glm::vec3> {};

	struct OpenGl_uniform_block_light_instance
	{
		glm::vec4 m_position_and_unused;
		glm::vec4 m_color_and_intensity;
	};

	template <>
	constexpr GLuint uniform_block_alignment_functions::get_alignment<OpenGl_uniform_block_light_instance[100]>()
	{
		return sizeof(OpenGl_uniform_block_light_instance) * 100;
	}

	struct OpenGl_uniform_block_lights : OpenGl_static_uniform_block<OpenGl_uniform_block_lights, uniform_block_names::block_lights, GL_STATIC_DRAW, GLfloat, OpenGl_uniform_block_light_instance[100]> {};
}
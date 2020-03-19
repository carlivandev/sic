#pragma once
#include "sic/type_restrictions.h"
#include "sic/gl_includes.h"

#include "glm/vec2.hpp"

namespace sic
{
	struct OpenGl_texture : Noncopyable
	{
		OpenGl_texture(const glm::ivec2& in_dimensions, GLuint in_channel_format, unsigned char* in_data);
		~OpenGl_texture();

		OpenGl_texture(OpenGl_texture&& in_other) noexcept;
		OpenGl_texture& operator=(OpenGl_texture&& in_other)  noexcept;

		void bind(GLuint in_uniform_location, GLuint in_active_texture_index);

	private:
		void free_resources();

		GLuint m_target_type = 0;
		GLuint m_dimension_format = 0;
		GLuint m_channel_format = 0;
		GLuint m_texture_id = 0;
	};
}
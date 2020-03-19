#pragma once
#include "sic/gl_includes.h"
#include "sic/type_restrictions.h"

#include <string>

namespace sic
{
	struct OpenGl_program : Noncopyable
	{
		OpenGl_program(const std::string& in_vertex_shader_name, const std::string& in_vertex_shader_code, const std::string& in_fragment_shader_name, const std::string& in_fragment_shader_code);
		~OpenGl_program();

		OpenGl_program(OpenGl_program&& in_other) noexcept;
		OpenGl_program& operator=(OpenGl_program&& in_other)  noexcept;

		void use() const;
		GLuint get_uniform_location(const char* in_key) const;

	private:
		void free_resources();

		GLuint m_program_id = 0;
	};
}
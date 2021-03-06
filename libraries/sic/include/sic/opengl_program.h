#pragma once
#include "sic/gl_includes.h"
#include "sic/opengl_vertex_buffer_array.h"
#include "sic/opengl_uniform_block.h"

#include "sic/core/type_restrictions.h"
#include "sic/core/logger.h"

#include "glm/mat4x4.hpp"
#include "glm/vec3.hpp"

#include <string>
#include <unordered_map>
#include <optional>

namespace sic
{
	struct OpenGl_texture;

	struct OpenGl_program : Noncopyable
	{
		OpenGl_program(const std::string& in_vertex_shader_name, const std::string& in_vertex_shader_code, const std::string& in_fragment_shader_name, const std::string& in_fragment_shader_code);
		~OpenGl_program();

		OpenGl_program(OpenGl_program&& in_other) noexcept;
		OpenGl_program& operator=(OpenGl_program&& in_other)  noexcept;

		void use() const;

		bool set_uniform(const char* in_key, const glm::mat4& in_matrix) const;
		bool set_uniform(const char* in_key, const glm::vec3& in_vec3) const;
		bool set_uniform(const char* in_key, const glm::vec4& in_vec4) const;
		bool set_uniform(const char* in_key, const OpenGl_texture& in_texture) const;
		bool set_uniform_from_bindless_handle(const char* in_key, GLuint64 in_texture_handle) const;
		bool set_uniform_block(const OpenGl_uniform_block& in_block) const;

		std::optional<GLuint> get_uniform_location(const char* in_key) const;

		const std::unordered_map<std::string, GLuint>& get_uniforms() const { return m_uniforms; }
		const std::unordered_map<std::string, GLuint>& get_uniform_blocks() const { return m_uniform_blocks; }

		template <typename ...T_attributes>
		bool is_compatible_with(const OpenGl_vertex_buffer_array<T_attributes...>& in_vertex_buffer_array) const;

		bool get_is_valid() const { return m_is_valid; }

		GLint get_attribute_count() const { return m_attribute_count; }

	private:
		template <typename T_attribute_type>
		void is_same(T_attribute_type& inout_attribute, ui32& inout_vertex_buffer_attribute_index, bool& out_is_same) const;

		void free_resources();

		std::vector<std::string> m_attributes;
		std::unordered_map<std::string, GLuint> m_uniforms;
		std::unordered_map<std::string, GLuint> m_uniform_blocks;

		GLuint m_program_id = 0;
		GLint m_attribute_count = 0;
		static GLint m_currently_enabled_attribute_count;

		bool m_is_valid = false;
	};

	template<typename ...T_attributes>
	inline bool OpenGl_program::is_compatible_with(const OpenGl_vertex_buffer_array<T_attributes...>& in_vertex_buffer_array) const
	{
		if (std::tuple_size<decltype(in_vertex_buffer_array.m_attributes)>::value != m_attribute_count)
		{
			SIC_LOG_E(g_log_renderer_verbose, "Program NOT compatible with VBA. (attribute count mismatch)");
			return false;
		}

		ui32 vertex_buffer_attribute_index = 0;
		bool matches = true;

		std::apply
		(
			[this, &vertex_buffer_attribute_index, &matches](auto && ... args)
			{
				(is_same(args, vertex_buffer_attribute_index, matches), ...);
			},
			in_vertex_buffer_array.m_attributes
		);

		return matches;
	}
	template<typename T_attribute_type>
	inline void OpenGl_program::is_same(T_attribute_type& inout_attribute, ui32& inout_vertex_buffer_attribute_index, bool& out_is_same) const
	{
		GLint size;
		GLenum type;

		const GLsizei max_name_length = 32;
		std::string name;
		name.resize(max_name_length);

		GLsizei name_length;
		SIC_GL_CHECK(glGetActiveAttrib(m_program_id, (GLuint)inout_vertex_buffer_attribute_index, max_name_length, &name_length, &size, &type, &(name[0])));

		name.resize(name_length);

		if (inout_attribute.get_name() != name)
		{
			SIC_LOG_E(g_log_renderer_verbose, "Program NOT compatible with VBA. (name mismatch: {0} != {1})", inout_attribute.get_name(), name);
			out_is_same = false;
		}

		if (inout_attribute.get_shader_data_type() != type)
		{
			SIC_LOG_E(g_log_renderer_verbose, "Program NOT compatible with VBA. (type mismatch: {0} != {1})", inout_attribute.get_name(), name);
			out_is_same = false;
		}

		++inout_vertex_buffer_attribute_index;
	}
}
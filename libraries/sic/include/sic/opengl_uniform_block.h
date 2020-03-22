#pragma once
#include "sic/gl_includes.h"
#include "sic/type_restrictions.h"

#include "glm/mat4x4.hpp"
#include "glm/vec3.hpp"

namespace sic
{
	struct OpenGl_uniform_block_base : Noncopyable
	{
		friend struct OpenGl_uniform_block_cache;

		virtual ~OpenGl_uniform_block_base() = default;

		GLuint get_block_id() const { return m_block_buffer_id; }
		GLuint get_block_size() const { return m_buffer_size; }
		GLuint get_binding_point() const { return m_binding_point; }

	protected:
		OpenGl_uniform_block_base() = default;

		GLuint m_block_buffer_id = 0;
		GLuint m_buffer_size = 0;
		GLuint m_binding_point = 0;
	};

	template <typename t_sub_type, const char* t_name, GLuint t_usage, typename ...t_uniform_types>
	struct OpenGl_uniform_block : OpenGl_uniform_block_base
	{
		static constexpr const char* get_name() { return t_name; }
		static constexpr GLuint get_usage() { return t_usage; }

		static t_sub_type& get()
		{
			static t_sub_type instance;
			return instance;
		}

		OpenGl_uniform_block()
		{
			((m_buffer_size += get_alignment<t_uniform_types>()), ...);

			glGenBuffers(1, &m_block_buffer_id);
			glBindBuffer(GL_UNIFORM_BUFFER, m_block_buffer_id);
			glBufferData(GL_UNIFORM_BUFFER, m_buffer_size, NULL, t_usage);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);
		}

		~OpenGl_uniform_block()
		{
			free_resources();
		}

		OpenGl_uniform_block(OpenGl_uniform_block&& in_other) noexcept
		{
			free_resources();
			m_block_buffer_id = in_other.m_block_buffer_id;
			m_buffer_size = in_other.m_buffer_size;

			in_other.m_block_buffer_id = 0;
		}

		OpenGl_uniform_block& operator=(OpenGl_uniform_block&& in_other) noexcept
		{
			free_resources();
			m_block_buffer_id = in_other.m_block_buffer_id;
			m_buffer_size = in_other.m_buffer_size;

			in_other.m_block_buffer_id = 0;

			return *this;
		}

		template <typename t_data_type>
		void set_data(ui32 in_index, const t_data_type& in_data)
		{
			ui32 current_index = 0;
			GLuint current_offset = 0;

			(set_data_on_index<t_data_type, t_uniform_types>(in_index, in_data, current_index, current_offset), ...);
		}

	private:
		template <typename t_data_type, typename t_uniform_type>
		constexpr void set_data_on_index(ui32 in_index, const t_data_type& in_data, ui32& inout_current_index, GLuint& inout_current_offset)
		{
			if (inout_current_index != in_index)
			{
				++inout_current_index;
				inout_current_offset += get_alignment<t_uniform_type>();

				return;
			}

			glBindBuffer(GL_UNIFORM_BUFFER, m_block_buffer_id);
			glBufferSubData(GL_UNIFORM_BUFFER, inout_current_offset, sizeof(t_data_type), glm::value_ptr(in_data));
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			++inout_current_index;
		}
		template <typename t>
		constexpr GLuint get_alignment()
		{
			static_assert(false, "Alignment has not been implemented for this type!");
			return 0;
		}

		template <>
		constexpr GLuint get_alignment<GLfloat>()
		{
			return 4;
		}

		template <>
		constexpr GLuint get_alignment<GLint>()
		{
			return 4;
		}

		template <>
		constexpr GLuint get_alignment<GLboolean>()
		{
			return 4;
		}

		template <>
		constexpr GLuint get_alignment<glm::vec2>()
		{
			return 8;
		}

		template <>
		constexpr GLuint get_alignment<glm::vec3>()
		{
			return 16;
		}

		template <>
		constexpr GLuint get_alignment<glm::vec4>()
		{
			return 16;
		}

		template <>
		constexpr GLuint get_alignment<glm::mat4x4>()
		{
			return 64;
		}

		void free_resources()
		{
			if (m_block_buffer_id != 0)
			{
				glDeleteBuffers(1, &m_block_buffer_id);
				m_block_buffer_id = 0;
			}
		}
	};

	namespace uniform_block_names
	{
		constexpr char block_view[] = "block_view";
		constexpr char block_test[] = "block_test";
	}

	struct OpenGl_uniform_block_view : OpenGl_uniform_block<OpenGl_uniform_block_view, uniform_block_names::block_view, GL_DYNAMIC_DRAW, glm::mat4x4> {};
	struct OpenGl_uniform_block_test : OpenGl_uniform_block<OpenGl_uniform_block_view, uniform_block_names::block_test, GL_STATIC_DRAW, glm::vec3, glm::vec3> {};
}
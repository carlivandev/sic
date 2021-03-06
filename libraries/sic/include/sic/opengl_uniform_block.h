#pragma once
#include "sic/gl_includes.h"

#include "sic/core/type_restrictions.h"

#include "glm/mat4x4.hpp"
#include "glm/vec3.hpp"

#include <string>
#include <vector>
#include <typeindex>

namespace sic
{
	namespace uniform_block_alignment_functions
	{
		template <typename t>
		constexpr GLuint get_alignment()
		{
			static_assert(false, "Alignment has not been implemented for this type!");
			return 0;
		}
	}
	
	struct OpenGl_uniform_block
	{
		struct Uniform
		{
			Uniform(std::type_index in_type_index, ui32 in_byte_offset) :
				m_type_index(in_type_index),
				m_byte_offset(in_byte_offset)
			{}

			std::type_index m_type_index;
			ui32 m_byte_offset;
		};

		GLuint get_block_id() const { return m_block_buffer_id; }
		GLuint get_block_size() const { return m_buffer_size; }
		GLuint get_binding_point() const { return m_binding_point; }

		const std::string& get_name() const { return m_name; }
		GLuint get_usage() const { return m_usage; }

		OpenGl_uniform_block(const char* in_name, GLuint in_usage, ui32 in_buffer_size) : m_name(in_name), m_usage(in_usage), m_binding_point(calculate_binding_point())
		{
			m_buffer_size = in_buffer_size;

			glGenBuffers(1, &m_block_buffer_id);
			glBindBuffer(GL_UNIFORM_BUFFER, m_block_buffer_id);
			glBufferData(GL_UNIFORM_BUFFER, m_buffer_size, NULL, m_usage);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);
		}

		virtual ~OpenGl_uniform_block()
		{
			free_resources();
		}

		OpenGl_uniform_block(OpenGl_uniform_block&& in_other) noexcept
		{
			free_resources();
			m_block_buffer_id = in_other.m_block_buffer_id;
			m_buffer_size = in_other.m_buffer_size;
			m_uniforms = std::move(in_other.m_uniforms);
			m_binding_point = in_other.m_binding_point;
			m_name = std::move(in_other.m_name);
			m_usage = in_other.m_usage;

			in_other.m_block_buffer_id = 0;
		}

		OpenGl_uniform_block& operator=(OpenGl_uniform_block&& in_other) noexcept
		{
			free_resources();
			m_block_buffer_id = in_other.m_block_buffer_id;
			m_buffer_size = in_other.m_buffer_size;
			m_uniforms = std::move(in_other.m_uniforms);
			m_binding_point = in_other.m_binding_point;
			m_name = std::move(in_other.m_name);
			m_usage = in_other.m_usage;

			in_other.m_block_buffer_id = 0;

			return *this;
		}

		template <typename T_data_type>
		void set_data(ui32 in_index, const T_data_type& in_data)
		{
			const Uniform& uniform = m_uniforms[in_index];

			assert(uniform.m_type_index == std::type_index(typeid(T_data_type)) && "Type mismatch when setting data!");
			
			SIC_GL_CHECK(glBindBuffer(GL_UNIFORM_BUFFER, m_block_buffer_id));
			SIC_GL_CHECK(glBufferSubData(GL_UNIFORM_BUFFER, uniform.m_byte_offset, sizeof(T_data_type), glm::value_ptr(in_data)));
			glBindBuffer(GL_UNIFORM_BUFFER, 0);
		}

		void set_data(ui32 in_index, GLfloat in_data)
		{
			assert(m_uniforms[in_index].m_type_index == std::type_index(typeid(GLfloat)) && "Type mismatch when setting data!");
			set_data_raw(in_index, 0, sizeof(GLfloat), &in_data);
		}

		void set_data_raw(ui32 in_uniform_index, ui32 in_byte_offset, ui32 in_byte_size, void* in_data)
		{
			const Uniform& uniform = m_uniforms[in_uniform_index];
			SIC_GL_CHECK(glBindBuffer(GL_UNIFORM_BUFFER, m_block_buffer_id));
			SIC_GL_CHECK(glBufferSubData(GL_UNIFORM_BUFFER, (ui64)uniform.m_byte_offset + in_byte_offset, in_byte_size, in_data));
			glBindBuffer(GL_UNIFORM_BUFFER, 0);
		}

		void set_data_raw(ui32 in_byte_offset, ui32 in_byte_size, void* in_data)
		{
			SIC_GL_CHECK(glBindBuffer(GL_UNIFORM_BUFFER, m_block_buffer_id));
			SIC_GL_CHECK(glBufferSubData(GL_UNIFORM_BUFFER, in_byte_offset, in_byte_size, in_data));
			glBindBuffer(GL_UNIFORM_BUFFER, 0);
		}

	protected:
		template <typename T_uniform_type>
		constexpr void add_uniform(GLuint& inout_current_offset)
		{
			m_uniforms.emplace_back(std::type_index(typeid(T_uniform_type)), inout_current_offset);

			SIC_GL_CHECK(glBindBuffer(GL_UNIFORM_BUFFER, m_block_buffer_id));
			SIC_GL_CHECK(glBufferSubData(GL_UNIFORM_BUFFER, inout_current_offset, sizeof(T_uniform_type), NULL));
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			inout_current_offset += uniform_block_alignment_functions::get_alignment<T_uniform_type>();
		}

	private:
		static std::vector<GLuint>& get_free_binding_points()
		{
			static std::vector<GLuint> free_binding_points;
			return free_binding_points;
		}

		GLuint calculate_binding_point() const
		{
			static GLuint ticker = 0;

			if (!get_free_binding_points().empty())
			{
				GLuint ret_val = get_free_binding_points().back();
				get_free_binding_points().pop_back();
				return ret_val;
			}

			return ticker++;
		}

		void free_resources()
		{
			if (m_block_buffer_id != 0)
			{
				SIC_GL_CHECK(glDeleteBuffers(1, &m_block_buffer_id));
				m_block_buffer_id = 0;

				get_free_binding_points().push_back(m_binding_point);
			}

			m_uniforms.clear();
		}

		std::string m_name;
		GLuint m_usage = 0;
		std::vector<Uniform> m_uniforms;

		GLuint m_block_buffer_id = 0;
		GLuint m_buffer_size = 0;
		GLuint m_binding_point = 0;
	};

	template <typename T_subtype, const char* T_name, GLuint T_usage, typename ...T_uniform_types>
	struct OpenGl_static_uniform_block : OpenGl_uniform_block
	{
		static const char* get_static_name() { return T_name; }

		OpenGl_static_uniform_block() : OpenGl_uniform_block(T_name, T_usage, calculate_buffer_size())
		{
			GLuint current_offset = 0;
			((add_uniform<T_uniform_types>(current_offset)), ...);
		}

	private:
		ui32 calculate_buffer_size() const
		{
			ui32 buffer_size = 0;
			((buffer_size += uniform_block_alignment_functions::get_alignment<T_uniform_types>()), ...);
			return buffer_size;
		}
	};

	template <>
	constexpr GLuint uniform_block_alignment_functions::get_alignment<GLfloat>()
	{
		return 16;
	}

	template <>
	constexpr GLuint uniform_block_alignment_functions::get_alignment<GLboolean>()
	{
		return 16;
	}

	template <>
	constexpr GLuint uniform_block_alignment_functions::get_alignment<GLuint64>()
	{
		return 16;
	}
	
	template <>
	constexpr GLuint uniform_block_alignment_functions::get_alignment<glm::vec2>()
	{
		return 16;
	}

	template <>
	constexpr GLuint uniform_block_alignment_functions::get_alignment<glm::vec3>()
	{
		return 16;
	}

	template <>
	constexpr GLuint uniform_block_alignment_functions::get_alignment<glm::vec4>()
	{
		return 16;
	}

	template <>
	constexpr GLuint uniform_block_alignment_functions::get_alignment<glm::mat4x4>()
	{
		return 64;
	}
}
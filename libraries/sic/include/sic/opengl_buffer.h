#pragma once
#include "sic/defines.h"
#include "sic/gl_includes.h"
#include "sic/type_restrictions.h"

#include <assert.h>
#include <vector>

namespace sic
{
	struct OpenGl_buffer : Noncopyable
	{
		OpenGl_buffer(GLenum in_target_type, GLenum in_usage_type) : m_target_type(in_target_type), m_usage_type(in_usage_type)
		{
			SIC_GL_CHECK(glGenBuffers(1, &m_buffer_id));
		}

		~OpenGl_buffer()
		{
			free_resources();
		}

		OpenGl_buffer(OpenGl_buffer&& in_other) noexcept
		{
			m_max_elements = in_other.m_max_elements;
			m_data_type_size = in_other.m_data_type_size;
			m_target_type = in_other.m_target_type;
			m_usage_type = in_other.m_usage_type;

			free_resources();

			if (in_other.m_buffer_id != 0)
			{
				m_buffer_id = in_other.m_buffer_id;
				in_other.m_buffer_id = 0;
			}

			in_other.m_data_type_size = 0;
		}

		OpenGl_buffer& operator=(OpenGl_buffer&& in_other)  noexcept
		{
			m_max_elements = in_other.m_max_elements;
			m_data_type_size = in_other.m_data_type_size;
			m_target_type = in_other.m_target_type;
			m_usage_type = in_other.m_usage_type;

			free_resources();

			if (in_other.m_buffer_id != 0)
			{
				m_buffer_id = in_other.m_buffer_id;
				in_other.m_buffer_id = 0;
			}

			in_other.m_data_type_size = 0;

			return *this;
		}

		void set_target_type(GLenum in_target_type)
		{
			m_target_type = in_target_type;
		}

		void set_usage_type(GLenum in_usage_type)
		{
			m_usage_type = in_usage_type;
		}

		template <typename T_data_type>
		void resize(ui32 in_max_elements)
		{
			m_data_type_size = sizeof(T_data_type);
			m_max_elements = in_max_elements;
			bind();

			SIC_GL_CHECK(glBufferData(m_target_type, (ui64)m_data_type_size * in_max_elements, nullptr, m_usage_type));
		}

		template <typename T_data_type>
		void set_data(const std::vector<T_data_type>& in_data)
		{
			m_data_type_size = sizeof(T_data_type);
			bind();

			if (m_max_elements != in_data.size())
				resize<T_data_type>(static_cast<ui32>(in_data.size()));

			SIC_GL_CHECK(glBufferSubData(m_target_type, 0, (ui64)m_data_type_size * m_max_elements, in_data.data()));
		}

		//doesn't resize
		template <typename T_data_type>
		void set_data_partial(const std::vector<T_data_type>& in_data, ui32 in_element_offset)
		{
			assert(m_data_type_size == sizeof(T_data_type) && "Data to set has size mismatch!");
			assert(in_data.size() + in_element_offset <= m_max_elements && "Data to set overflowed buffer!");

			bind();

			SIC_GL_CHECK(glBufferSubData(m_target_type, m_data_type_size * in_element_offset, (ui64)m_data_type_size * m_max_elements, in_data.data()));
		}

		void set_data_raw(const void* in_data, ui32 in_offset, ui32 in_bytesize)
		{
			assert(in_offset + in_bytesize <= m_max_elements * m_data_type_size && "Data to set overflowed buffer!");

			bind();

			SIC_GL_CHECK(glBufferSubData(m_target_type, in_offset, in_bytesize, in_data));
		}

		void bind() const
		{
			SIC_GL_CHECK(glBindBuffer(m_target_type, m_buffer_id));
		}

		ui32 get_max_elements() const { return m_max_elements; }
		ui32 get_buffer_id() const { return m_buffer_id; }

	private:
		void free_resources()
		{
			if (m_buffer_id != 0)
				SIC_GL_CHECK(glDeleteBuffers(1, &m_buffer_id));
		}

		ui32 m_buffer_id = 0;
		ui32 m_max_elements = 0;
		ui32 m_data_type_size = 0;

		GLenum m_target_type = 0;
		GLenum m_usage_type = 0;
	};
}
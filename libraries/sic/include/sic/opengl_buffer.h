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
			glGenBuffers(1, &m_buffer_id);
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

		template <typename t_data_type>
		void resize(ui32 in_max_elements)
		{
			m_data_type_size = sizeof(t_data_type);
			m_max_elements = in_max_elements;
			bind();

			glBufferData(m_target_type, (ui64)m_data_type_size * in_max_elements, nullptr, m_usage_type);
		}

		template <typename t_data_type>
		void set_data(const std::vector<t_data_type>& in_data)
		{
			m_data_type_size = sizeof(t_data_type);
			bind();

			if (m_max_elements != in_data.size())
				resize<t_data_type>(static_cast<ui32>(in_data.size()));

			glBufferSubData(m_target_type, 0, (ui64)m_data_type_size * m_max_elements, in_data.data());
		}

		//doesn't resize
		template <typename t_data_type>
		void set_data_partial(const std::vector<t_data_type>& in_data, ui32 in_element_offset)
		{
			assert(m_data_type_size == sizeof(t_data_type) && "Data to set has size mismatch!");
			assert(in_data.size() + in_element_offset <= m_max_elements && "Data to set overflowed buffer!");

			bind();

			glBufferSubData(m_target_type, m_data_type_size * in_element_offset, (ui64)m_data_type_size * m_max_elements, in_data.data());
		}

		void bind() const
		{
			glBindBuffer(m_target_type, m_buffer_id);
		}

		ui32 get_max_elements() const { return m_max_elements; }

	private:
		void free_resources()
		{
			if (m_buffer_id != 0)
				glDeleteBuffers(1, &m_buffer_id);
		}

		ui32 m_buffer_id = 0;
		ui32 m_max_elements = 0;
		ui32 m_data_type_size = 0;

		GLenum m_target_type = 0;
		GLenum m_usage_type = 0;
	};
}
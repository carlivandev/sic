#pragma once
#include "sic/gl_includes.h"

#include "sic/core/defines.h"
#include "sic/core/type_restrictions.h"

#include <assert.h>
#include <vector>

namespace sic
{
	enum class OpenGl_buffer_target
	{
		invalid = -1,

		array = GL_ARRAY_BUFFER,
		atomic_counter = GL_ATOMIC_COUNTER_BUFFER,
		copy_read = GL_COPY_READ_BUFFER,
		copy_write = GL_COPY_WRITE_BUFFER,
		dispatch_indirect = GL_DISPATCH_INDIRECT_BUFFER,
		draw_indirect = GL_DRAW_INDIRECT_BUFFER,
		element_array = GL_ELEMENT_ARRAY_BUFFER,
		pixel_pack = GL_PIXEL_PACK_BUFFER,
		pixel_unpack = GL_PIXEL_UNPACK_BUFFER,
		query = GL_QUERY_BUFFER,
		shader_storage = GL_SHADER_STORAGE_BUFFER,
		texture = GL_TEXTURE_BUFFER,
		transform_feedback = GL_TRANSFORM_FEEDBACK_BUFFER,
		uniform = GL_UNIFORM_BUFFER
	};

	enum class OpenGl_buffer_usage
	{
		invalid = -1,

		static_draw = GL_STATIC_DRAW,
		static_read = GL_STATIC_READ,
		static_copy = GL_STATIC_COPY,

		dynamic_draw = GL_DYNAMIC_DRAW,
		dynamic_read = GL_DYNAMIC_READ,
		dynamic_copy = GL_DYNAMIC_COPY,

		stream_draw = GL_STREAM_DRAW,
		stream_read = GL_STREAM_READ,
		stream_copy = GL_STREAM_COPY
	};

	struct OpenGl_buffer : Noncopyable
	{
		struct Creation_params
		{
			Creation_params(OpenGl_buffer_target in_target, OpenGl_buffer_usage in_usage) : m_target(in_target), m_usage(in_usage) {}
			OpenGl_buffer_target m_target = OpenGl_buffer_target::invalid;
			OpenGl_buffer_usage m_usage = OpenGl_buffer_usage::invalid;
		};

		OpenGl_buffer(const Creation_params& in_params) : m_target(in_params.m_target), m_usage(in_params.m_usage)
		{
			SIC_GL_CHECK(glGenBuffers(1, &m_buffer_id));
		}

		OpenGl_buffer(Creation_params&& in_params) : m_target(in_params.m_target), m_usage(in_params.m_usage)
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
			m_target = in_other.m_target;
			m_usage = in_other.m_usage;

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
			m_target = in_other.m_target;
			m_usage = in_other.m_usage;

			free_resources();

			if (in_other.m_buffer_id != 0)
			{
				m_buffer_id = in_other.m_buffer_id;
				in_other.m_buffer_id = 0;
			}

			in_other.m_data_type_size = 0;

			return *this;
		}

		void set_target(OpenGl_buffer_target in_target)
		{
			m_target = in_target;
		}

		void set_usage(OpenGl_buffer_usage in_usage)
		{
			m_usage = in_usage;
		}

		template <typename T_data_type>
		void resize(ui32 in_max_elements)
		{
			m_data_type_size = sizeof(T_data_type);
			m_max_elements = in_max_elements;
			bind();

			SIC_GL_CHECK(glBufferData(static_cast<GLenum>(m_target), (ui64)m_data_type_size * in_max_elements, nullptr, static_cast<GLenum>(m_usage)));
		}

		template <typename T_data_type>
		void set_data(const std::vector<T_data_type>& in_data)
		{
			m_data_type_size = sizeof(T_data_type);
			bind();

			if (m_max_elements != in_data.size())
				resize<T_data_type>(static_cast<ui32>(in_data.size()));

			SIC_GL_CHECK(glBufferSubData(static_cast<GLenum>(m_target), 0, (ui64)m_data_type_size * m_max_elements, in_data.data()));
		}

		//doesn't resize
		template <typename T_data_type>
		void set_data_partial(const std::vector<T_data_type>& in_data, ui32 in_element_offset)
		{
			assert(m_data_type_size == sizeof(T_data_type) && "Data to set has size mismatch!");
			assert(in_data.size() + in_element_offset <= m_max_elements && "Data to set overflowed buffer!");

			bind();

			SIC_GL_CHECK(glBufferSubData(static_cast<GLenum>(m_target), m_data_type_size * in_element_offset, (ui64)m_data_type_size * m_max_elements, in_data.data()));
		}

		void set_data_raw(const void* in_data, ui32 in_offset, ui32 in_bytesize)
		{
			assert(in_offset + in_bytesize <= m_max_elements * m_data_type_size && "Data to set overflowed buffer!");

			bind();

			SIC_GL_CHECK(glBufferSubData(static_cast<GLenum>(m_target), in_offset, in_bytesize, in_data));
		}

		void bind() const
		{
			SIC_GL_CHECK(glBindBuffer(static_cast<GLenum>(m_target), m_buffer_id));
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

		OpenGl_buffer_target m_target = OpenGl_buffer_target::invalid;
		OpenGl_buffer_usage m_usage = OpenGl_buffer_usage::invalid;
	};
}
#pragma once
#include "sic/opengl_buffer.h"
#include "sic/opengl_vertex_attributes.h"

#include <tuple>

namespace sic
{
	template <typename ...t_attributes>
	struct OpenGl_vertex_buffer_array : Noncopyable
	{
		OpenGl_vertex_buffer_array()
		{
			glGenVertexArrays(1, &m_id);
			bind();

			ui32 index = 0;
			std::apply([&index](auto&& ... args) {(enable_attribute(args, index), ...); }, m_attributes);
		}

		~OpenGl_vertex_buffer_array()
		{
			glDeleteVertexArrays(1, &m_id);
		}

		OpenGl_vertex_buffer_array(OpenGl_vertex_buffer_array&& in_other)
		{
			if (m_id != 0)
				glDeleteVertexArrays(1, &m_id);

			if (in_other.m_id != 0)
			{
				m_id = in_other.m_id;
				in_other.m_id = 0;
			}

			m_attributes = std::move(in_other.m_attributes);
		}
		OpenGl_vertex_buffer_array& operator=(OpenGl_vertex_buffer_array&& in_other)
		{
			if (m_id != 0)
				glDeleteVertexArrays(1, &m_id);

			if (in_other.m_id != 0)
			{
				m_id = in_other.m_id;
				in_other.m_id = 0;
			}

			m_attributes = std::move(in_other.m_attributes);

			return *this;
		}

		void bind() const
		{
			glBindVertexArray(m_id);
		}

		template <typename t_attribute_type, typename t_data_type>
		void set_data(const std::vector<t_data_type>& in_data)
		{
			bind();
			std::get<t_attribute_type>(m_attributes).m_buffer.set_data<t_data_type>(in_data);
		}

		template <typename t_attribute_type>
		static void enable_attribute(t_attribute_type& inout_attribute, ui32& inout_index)
		{
			static_assert(std::is_base_of<OpenGl_vertex_attribute_base, t_attribute_type>::value, "This is not a valid attribute type. see <opengl_vertex_attributes.h> for all attribute types.");
			glEnableVertexAttribArray(inout_index);
			inout_attribute.m_buffer.bind();
			glVertexAttribPointer(inout_index, t_attribute_type::get_size(), t_attribute_type::get_type(), GL_FALSE, 0, (void*)0);

			++inout_index;
		}

		std::tuple<t_attributes...> m_attributes;
		ui32 m_id = 0;
	};
}
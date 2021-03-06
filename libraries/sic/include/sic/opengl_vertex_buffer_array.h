#pragma once
#include "sic/opengl_buffer.h"
#include "sic/opengl_vertex_attributes.h"

#include "sic/core/logger.h"

#include <tuple>

namespace sic
{
	constexpr ui32 max_vertex_attributes = 16;

	struct OpenGl_vertex_buffer_array_base : Noncopyable
	{
		void bind() const
		{
			SIC_GL_CHECK(glBindVertexArray(m_id));
		}

		ui32 m_id = 0;
	};

	template <typename ...T_attributes>
	struct OpenGl_vertex_buffer_array : OpenGl_vertex_buffer_array_base
	{
		OpenGl_vertex_buffer_array()
		{
			SIC_GL_CHECK(glGenVertexArrays(1, &m_id));
			bind();

			ui32 index = 0;
			std::apply([&index](auto&& ... args) {(enable_attribute(args, index), ...); }, m_attributes);
		}

		~OpenGl_vertex_buffer_array()
		{
			free_resources();
		}

		OpenGl_vertex_buffer_array(OpenGl_vertex_buffer_array&& in_other) noexcept
		{
			free_resources();

			if (in_other.m_id != 0)
			{
				m_id = in_other.m_id;
				in_other.m_id = 0;
			}

			m_attributes = std::move(in_other.m_attributes);
		}
		OpenGl_vertex_buffer_array& operator=(OpenGl_vertex_buffer_array&& in_other) noexcept
		{
			free_resources();

			if (in_other.m_id != 0)
			{
				m_id = in_other.m_id;
				in_other.m_id = 0;
			}

			m_attributes = std::move(in_other.m_attributes);

			return *this;
		}

		template <typename T_attribute_type, typename T_data_type>
		void resize(ui32 in_max_elements)
		{
			bind();
			std::get<T_attribute_type>(m_attributes).m_buffer.resize<T_data_type>(in_max_elements);
		}

		template <typename T_attribute_type, typename T_data_type>
		void set_data(const std::vector<T_data_type>& in_data)
		{
			bind();
			std::get<T_attribute_type>(m_attributes).m_buffer.set_data<T_data_type>(in_data);
		}

		//doesnt resize
		template <typename T_attribute_type, typename T_data_type>
		void set_data_partial(const std::vector<T_data_type>& in_data, ui32 in_element_offset)
		{
			bind();
			std::get<T_attribute_type>(m_attributes).m_buffer.set_data_partial<T_data_type>(in_data, in_element_offset);
		}

		template <typename T_attribute_type>
		static void enable_attribute(T_attribute_type& inout_attribute, ui32& inout_index)
		{
			static_assert(std::is_base_of<OpenGl_vertex_attribute_base, T_attribute_type>::value, "This is not a valid attribute type. see <opengl_vertex_attributes.h> for all attribute types.");
			SIC_GL_CHECK(glEnableVertexAttribArray(inout_index));
			inout_attribute.m_buffer.bind();
			SIC_GL_CHECK(glVertexAttribPointer(inout_index, T_attribute_type::get_size(), T_attribute_type::get_data_type(), GL_FALSE, 0, (void*)0));

			++inout_index;
		}

		constexpr size_t get_attribute_count() const { return std::tuple_size<std::tuple<T_attributes...>>::value; }

	private:
		void free_resources()
		{
			if (m_id != 0)
				SIC_GL_CHECK(glDeleteVertexArrays(1, &m_id));
		}

		std::tuple<T_attributes...> m_attributes;
	};
}
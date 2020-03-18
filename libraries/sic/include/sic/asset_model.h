#pragma once
#include "sic/asset.h"
#include "sic/defines.h"
#include "sic/gl_includes.h"
#include "sic/type_restrictions.h"

#include <string>
#include <vector>
#include <memory>

#include "glm/vec3.hpp"
#include "glm/vec2.hpp"

namespace sic
{
	struct Asset_material;

	struct OpenGl_buffer : Noncopyable
	{
		OpenGl_buffer(GLenum in_target_type, GLenum in_usage_type) : m_target_type(in_target_type), m_usage_type(in_usage_type)
		{
			glGenBuffers(1, &m_buffer_id);
		}

		~OpenGl_buffer()
		{
			glDeleteBuffers(1, &m_buffer_id);
		}

		void set_target_type(GLenum in_target_type)
		{
			m_target_type = in_target_type;
		}

		void set_usage_type(GLenum in_usage_type)
		{
			m_usage_type = in_usage_type;
		}

		void resize(ui32 in_new_size)
		{
			assert(m_data_type_size != 0 && "Must call set_data before resizing.");

			m_size = in_new_size;
			bind();
			glBufferData(m_target_type, m_data_type_size * m_size, nullptr, m_usage_type);
		}

		template <typename t_data_type>
		void set_data(const std::vector<t_data_type>& in_data)
		{
			m_data_type_size = sizeof(t_data_type);
			bind();

			if (m_size != in_data.size())
				resize(in_data.size());

			glBufferSubData(m_target_type, 0, m_data_type_size * m_size, in_data.data());
		}

		void bind()
		{
			glBindBuffer(m_target_type, m_buffer_id);
		}

		ui32 size() const { return m_size; }

	private:
		ui32 m_buffer_id = 0;
		ui32 m_size = 0;
		ui32 m_data_type_size = 0;

		GLenum m_target_type = -1;
		GLenum m_usage_type = -1;
	};

	struct OpenGl_vertex_attribute_base
	{
		OpenGl_vertex_attribute_base() : m_buffer(GL_ARRAY_BUFFER, GL_STATIC_DRAW)
		{

		}
		OpenGl_buffer m_buffer;
	};

	template <GLenum t_type, ui32 t_size>
	struct OpenGl_vertex_attribute : OpenGl_vertex_attribute_base
	{
		static constexpr GLenum get_type() { return t_type; }
		static constexpr ui32 get_size() { return t_size; }
	};

	struct OpenGl_vertex_attribute_float3 : OpenGl_vertex_attribute<GL_FLOAT, 3> {};
	struct OpenGl_vertex_attribute_float2 : OpenGl_vertex_attribute<GL_FLOAT, 2> {};

	template <typename ...t_attributes>
	struct OpenGl_vertex_buffer_array
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

		void bind()
		{
			glBindVertexArray(m_id);
		}

		template <ui32 t_index, typename t_data_type>
		void set_data(const std::vector<t_data_type>& in_data)
		{
			bind();
			std::get<t_index>(m_attributes).m_buffer.set_data<t_data_type>(in_data);
		}

		template <typename t_attribute_type>
		static void enable_attribute(t_attribute_type& inout_attribute, ui32& inout_index)
		{
			static_assert(std::is_base_of<OpenGl_vertex_attribute_base, t_attribute_type>::value, "This is not a valid attribute type. see <filename> for all attribute types.");
			glEnableVertexAttribArray(inout_index);
			inout_attribute.m_buffer.bind();
			glVertexAttribPointer(inout_index, t_attribute_type::get_size(), t_attribute_type::get_type(), GL_FALSE, 0, (void*)0);

			++inout_index;
		}

		std::tuple<t_attributes...> m_attributes;
		ui32 m_id = 0;
	};

	struct Asset_model : Asset
	{
		struct Mesh
		{
			//if vertex layout changes we NEED to update asset_management::load_mesh as it depends on this struct layout
			struct Vertex
			{
				glm::vec3 m_position;
				glm::vec3 m_normal;
				glm::vec2 m_tex_coords;
				glm::vec3 m_tangent;
				glm::vec3 m_bitangent;
			};

			std::vector<Vertex> m_vertices;
			std::vector<ui32> m_indices;
			std::string m_material_slot;

			ui32 m_vertexarray;
			ui32 m_vertexbuffer;
			ui32 m_elementbuffer;
		};

		bool has_post_load() override final { return true; }
		bool has_pre_unload() override final { return true; }

		void load(const State_assetsystem& in_assetsystem_state, Deserialize_stream& in_stream) override;
		void save(const State_assetsystem& in_assetsystem_state, Serialize_stream& out_stream) const override;
		bool import(std::string&& in_data);

		const Asset_ref<Asset_material>& get_material(i32 in_index) const;

		//returns -1 if invalid slot
		i32 get_slot_index(const char* in_material_slot) const;

		void set_material(const Asset_ref<Asset_material>& in_material, const char* in_material_slot);


		std::vector<std::pair<Mesh, Asset_ref<Asset_material>>> m_meshes;
	};
}
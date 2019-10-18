#pragma once
#include <string>
#include <vector>
#include <memory>

#include "impuls/defines.h"
#include "gl_includes.h"

#include "glm/vec3.hpp"
#include "glm/vec2.hpp"

namespace impuls
{
	enum class e_texture_format
	{
		invalid = 0,	/*STBI_default*/
		gray = 1,		/*STBI_grey*/
		gray_a = 2,		/*STBI_grey_alpha*/
		rgb = 3,		/*STBI_rgb*/
		rgb_a = 4		/*STBI_rgb_alpha*/
	};

	struct asset_texture
	{
		impuls::i32 m_width = 0;
		impuls::i32 m_height = 0;
		impuls::i32 m_channel_count = 0;

		e_texture_format format = e_texture_format::invalid;

		unsigned char* m_texture_data = nullptr;
		GLuint m_render_id = 0;
	};

	struct asset_material
	{
		struct texture_parameter
		{
			std::string m_name;
			std::shared_ptr<asset_texture> m_texture;
		};

		std::string m_vertex_shader;
		std::string m_fragment_shader;

		std::vector<texture_parameter> m_texture_parameters;

		GLuint m_program_id = 0;
	};

	struct asset_model
	{
		struct mesh
		{
			//if vertex layout changes we NEED to update asset_management::load_mesh as it depends on this struct layout
			struct vertex
			{
				glm::vec3 m_position;
				glm::vec3 m_normal;
				glm::vec2 m_tex_coords;
				glm::vec3 m_tangent;
				glm::vec3 m_bitangent;
			};

			std::vector<vertex> m_vertices;
			std::vector<ui32> m_indices;
			std::string material_slot;

			ui32 m_vao;
			ui32 m_vbo;
			ui32 m_ebo;
		};

		std::shared_ptr<asset_material> get_material(i32 in_index)
		{
			if (in_index < m_meshes.size())
			{
				return m_meshes[in_index].second;
			}

			return nullptr;
		}

		std::vector<std::pair<mesh, std::shared_ptr<asset_material>>> m_meshes;
	};
}
#pragma once
#include "impuls/asset_management.h"
#include "impuls/asset_types.h"
#include "impuls/defines.h"
#include "impuls/file_management.h"

#include "fmt/format.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include <fstream>
#include <sstream>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace impuls::asset_management
{
	std::shared_ptr<asset_material> load_material(const char* in_filepath)
	{
		std::shared_ptr<asset_material> ret_val;

		const std::string vertex_file_path = fmt::format("{0}/material.vs", in_filepath);
		const std::string fragment_file_path = fmt::format("{0}/material.fs", in_filepath);

		// Create the shaders
		GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
		GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

		const std::string vertex_shader_code = file_management::load_file(vertex_file_path, false);

		if (vertex_shader_code.size() == 0)
		{
			printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", vertex_file_path.c_str());
			return ret_val;
		}
		const std::string fragment_shader_code = file_management::load_file(fragment_file_path, false);

		if (fragment_shader_code.size() == 0)
		{
			printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", fragment_file_path.c_str());
			return ret_val;
		}

		GLint result_id = GL_FALSE;
		i32 info_log_length;

		// Compile Vertex Shader
		printf("Compiling shader : %s\n", vertex_file_path.c_str());
		const GLchar* vertex_source_ptr = vertex_shader_code.data();
		glShaderSource(vertex_shader_id, 1, &vertex_source_ptr, NULL);
		glCompileShader(vertex_shader_id);

		// Check Vertex Shader
		glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &result_id);
		glGetShaderiv(vertex_shader_id, GL_INFO_LOG_LENGTH, &info_log_length);
		if (info_log_length > 0)
		{
			std::vector<char> VertexShaderErrorMessage(info_log_length + 1);
			glGetShaderInfoLog(vertex_shader_id, info_log_length, NULL, &VertexShaderErrorMessage[0]);
			printf("%s\n", &VertexShaderErrorMessage[0]);
		}

		// Compile Fragment Shader
		printf("Compiling shader : %s\n", fragment_file_path.c_str());
		const GLchar* fragment_source_ptr = fragment_shader_code.data();
		glShaderSource(fragment_shader_id, 1, &fragment_source_ptr, NULL);
		glCompileShader(fragment_shader_id);

		// Check Fragment Shader
		glGetShaderiv(fragment_shader_id, GL_COMPILE_STATUS, &result_id);
		glGetShaderiv(fragment_shader_id, GL_INFO_LOG_LENGTH, &info_log_length);
		if (info_log_length > 0)
		{
			std::vector<char> fragment_shader_error_message(info_log_length + 1);
			glGetShaderInfoLog(fragment_shader_id, info_log_length, NULL, &fragment_shader_error_message[0]);
			printf("%s\n", &fragment_shader_error_message[0]);
		}

		// Link the program
		printf("Linking program\n");
		GLuint program_id = glCreateProgram();
		glAttachShader(program_id, vertex_shader_id);
		glAttachShader(program_id, fragment_shader_id);
		glLinkProgram(program_id);

		// Check the program
		glGetProgramiv(program_id, GL_LINK_STATUS, &result_id);
		glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &info_log_length);
		if (info_log_length > 0)
		{
			std::vector<char> program_error_message(info_log_length + 1);
			glGetProgramInfoLog(program_id, info_log_length, NULL, &program_error_message[0]);
			printf("%s\n", &program_error_message[0]);
		}

		glDetachShader(program_id, vertex_shader_id);
		glDetachShader(program_id, fragment_shader_id);

		glDeleteShader(vertex_shader_id);
		glDeleteShader(fragment_shader_id);

		ret_val = std::make_shared<asset_material>();
		ret_val->m_vertex_shader = vertex_file_path;
		ret_val->m_fragment_shader = fragment_file_path;
		ret_val->m_program_id = program_id;

		return ret_val;
	}

	std::shared_ptr<asset_texture> load_texture(const char* in_filepath, e_texture_format in_format)
	{
		std::shared_ptr<asset_texture> ret_val;

		impuls::i32 width, height, channel_count;

		const std::string file_data = file_management::load_file(in_filepath);

		if (file_data.size() == 0)
			return ret_val;

		stbi_uc* texture_data = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(file_data.c_str()), static_cast<int>(file_data.size()), &width, &height, &channel_count, static_cast<impuls::i8>(in_format));
		
		if (!texture_data)
			return ret_val;

		if (channel_count != static_cast<impuls::i8>(in_format))
		{
			printf("texture load channel size mismatch, expected: %i, source: \"%s\"\n", channel_count, in_filepath);
			return ret_val;
		}

		ret_val = std::make_shared<asset_texture>();
		ret_val->m_width = width;
		ret_val->m_height = height;
		ret_val->m_channel_count = channel_count;
		ret_val->format = in_format;

		//currently we dont clean up stbi resource, in the future we want to decide whether or not to clean after GPU setup (for cpu texture modification shenanigans)
		ret_val->m_texture_data = texture_data;

		//begin gpu texture setup

		GLuint& texture_id = ret_val->m_render_id;

		glGenTextures(1, &texture_id);

		glBindTexture(GL_TEXTURE_2D, texture_id);

		// Nice trilinear filtering.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// When MAGnifying the image (no bigger mipmap available), use LINEAR filtering
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// When MINifying the image, use a LINEAR blend of two mipmaps, each filtered LINEARLY too
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

		i32 gl_texture_format = 0;

		switch (in_format)
		{
		case e_texture_format::gray:
			gl_texture_format = GL_R;
			break;
		case e_texture_format::gray_a:
			gl_texture_format = GL_RG;
			break;
		case e_texture_format::rgb:
			gl_texture_format = GL_RGB;
			break;
		case e_texture_format::rgb_a:
			gl_texture_format = GL_RGBA;
			break;
		default:
			break;
		}

		glTexImage2D(GL_TEXTURE_2D, 0, gl_texture_format, ret_val->m_width, ret_val->m_height, 0, gl_texture_format, GL_UNSIGNED_BYTE, ret_val->m_texture_data);

		glGenerateMipmap(GL_TEXTURE_2D);

		glBindTexture(GL_TEXTURE_2D, 0);
		//end gpu texture setup

		return ret_val;
	}

	namespace impuls_private
	{
		asset_model::mesh process_mesh(aiMesh* in_mesh, const aiScene* in_scene)
		{
			asset_model::mesh new_mesh;
			new_mesh.m_vertices.reserve(in_mesh->mNumVertices);

			for (unsigned int i = 0; i < in_mesh->mNumVertices; i++)
			{
				asset_model::mesh::vertex& new_vertex = new_mesh.m_vertices.emplace_back();

				glm::vec3 vert_pos;
				vert_pos.x = in_mesh->mVertices[i].x;
				vert_pos.y = in_mesh->mVertices[i].y;
				vert_pos.z = in_mesh->mVertices[i].z;
				new_vertex.m_position = vert_pos;

				glm::vec3 vert_normal;
				vert_normal.x = in_mesh->mNormals[i].x;
				vert_normal.y = in_mesh->mNormals[i].y;
				vert_normal.z = in_mesh->mNormals[i].z;
				new_vertex.m_normal = vert_normal;

				if (in_mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
				{
					glm::vec2 vert_texture_coord;
					vert_texture_coord.x = in_mesh->mTextureCoords[0][i].x;
					vert_texture_coord.y = in_mesh->mTextureCoords[0][i].y;
					new_vertex.m_tex_coords = vert_texture_coord;
				}
				else
					new_vertex.m_tex_coords = glm::vec2(0.0f, 0.0f);
			}

			// process indices
			for (ui32 i = 0; i < in_mesh->mNumFaces; i++)
			{
				aiFace& face = in_mesh->mFaces[i];
				for (ui32 j = 0; j < face.mNumIndices; j++)
					new_mesh.m_indices.push_back(face.mIndices[j]);
			}

			// process material
			if (in_mesh->mMaterialIndex >= 0)
			{
				aiMaterial* material = in_scene->mMaterials[in_mesh->mMaterialIndex];

				//we only store slot names
				//model then has LUT from material_slot to material, so its the materials that own textures
				new_mesh.material_slot = material->GetName().C_Str();
			}

			return std::move(new_mesh);
		}

		void process_node(aiNode* in_node, const aiScene* in_scene, std::vector<asset_model::mesh>& out_meshes)
		{
			// process all the node's meshes (if any)
			for (unsigned int i = 0; i < in_node->mNumMeshes; i++)
			{
				aiMesh* mesh = in_scene->mMeshes[in_node->mMeshes[i]];

				out_meshes.push_back(process_mesh(mesh, in_scene));
			}

			// then do the same for each of its children
			for (unsigned int i = 0; i < in_node->mNumChildren; i++)
				process_node(in_node->mChildren[i], in_scene, out_meshes);
		}

		void init_mesh(asset_model::mesh& out_mesh)
		{
			std::vector<asset_model::asset_model::mesh::vertex>& vertices = out_mesh.m_vertices;
			std::vector<ui32>& indices = out_mesh.m_indices;

			glGenVertexArrays(1, &out_mesh.m_vao);
			glGenBuffers(1, &out_mesh.m_vbo);
			glGenBuffers(1, &out_mesh.m_ebo);

			glBindVertexArray(out_mesh.m_vao);
			glBindBuffer(GL_ARRAY_BUFFER, out_mesh.m_vbo);

			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(asset_model::mesh::vertex), &vertices[0], GL_STATIC_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, out_mesh.m_ebo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(ui32), &indices[0], GL_STATIC_DRAW);

			// vertex positions
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(asset_model::mesh::vertex), (void*)0);
			// vertex normals
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(asset_model::mesh::vertex), (void*)offsetof(asset_model::mesh::vertex, m_normal));
			// vertex texture coords
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(asset_model::mesh::vertex), (void*)offsetof(asset_model::mesh::vertex, m_tex_coords));
			// vertex tangent
			glEnableVertexAttribArray(3);
			glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(asset_model::mesh::vertex), (void*)offsetof(asset_model::mesh::vertex, m_tangent));
			// vertex bitangent
			glEnableVertexAttribArray(4);
			glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(asset_model::mesh::vertex), (void*)offsetof(asset_model::mesh::vertex, m_bitangent));

			glBindVertexArray(0);
		}
	}

	std::shared_ptr<asset_model> load_model(const char* in_filepath)
	{
		std::shared_ptr<asset_model> ret_val;

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(in_filepath, aiProcess_Triangulate | aiProcess_FlipUVs);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			printf("ERROR::ASSIMP::%s\n", importer.GetErrorString());
			return ret_val;
		}

		std::vector<asset_model::mesh> meshes_to_load;
		impuls_private::process_node(scene->mRootNode, scene, meshes_to_load);

		if (meshes_to_load.empty())
			return ret_val;

		ret_val = std::make_shared<asset_model>();

		for (ui32 i = 0; i < meshes_to_load.size(); i++)
		{
			ret_val->m_meshes.push_back({ meshes_to_load[i], nullptr });
			impuls_private::init_mesh(ret_val->m_meshes.back().first);
		}

		return ret_val;
	}
}
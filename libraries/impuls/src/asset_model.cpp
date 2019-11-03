#pragma once
#include "impuls/asset_model.h"
#include "impuls/defines.h"
#include "impuls/file_management.h"
#include "impuls/system_asset.h"
#include "impuls/asset_material.h"

#include "fmt/format.h"

#include <fstream>
#include <sstream>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace impuls_private
{
	using namespace impuls;

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
}

void impuls::asset_model::load(const state_assetsystem& in_assetsystem_state, deserialize_stream& in_stream)
{
	size_t len = 0;
	in_stream.read(len);

	m_meshes.resize(len);

	for (ui32 i = 0; i < len; i++)
	{
		auto& mesh = m_meshes[i];

		in_stream.read(mesh.first.m_vertices);
		in_stream.read(mesh.first.m_indices);
		in_stream.read(mesh.first.material_slot);

		std::string header_id;
		in_stream.read(header_id);

		if (!header_id.empty())
			mesh.second = in_assetsystem_state.find_asset<asset_material>(xg::Guid(header_id));
	}
}

void impuls::asset_model::save(const state_assetsystem& in_assetsystem_state, serialize_stream& out_stream) const
{
	in_assetsystem_state;

	out_stream.write(m_meshes.size());

	for (ui32 i = 0; i < m_meshes.size(); i++)
	{
		const auto& mesh = m_meshes[i];

		out_stream.write(mesh.first.m_vertices);
		out_stream.write(mesh.first.m_indices);
		out_stream.write(mesh.first.material_slot);

		out_stream.write(mesh.second.is_valid() ? mesh.second.get_header()->m_id.str() : std::string(""));
	}
}

bool impuls::asset_model::import(std::string&& in_data)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFileFromMemory(in_data.data(), in_data.size(), aiProcess_Triangulate | aiProcess_FlipUVs);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		printf("ERROR::ASSIMP::%s\n", importer.GetErrorString());
		return false;
	}

	std::vector<asset_model::mesh> meshes_to_load;
	impuls_private::process_node(scene->mRootNode, scene, meshes_to_load);

	if (meshes_to_load.empty())
		return false;

	for (ui32 i = 0; i < meshes_to_load.size(); i++)
		m_meshes.push_back({ meshes_to_load[i], nullptr });

	return true;
}

impuls::asset_ref<impuls::asset_material> impuls::asset_model::get_material(i32 in_index) const
{
	if (in_index < m_meshes.size())
	{
		return m_meshes[in_index].second;
	}

	return nullptr;
}

impuls::i32 impuls::asset_model::get_slot_index(const char* in_material_slot) const
{
	const ui64 mesh_count = m_meshes.size();

	for (i32 i = 0; i < mesh_count; i++)
	{
		auto& mesh = m_meshes[i];
		if (mesh.first.material_slot == in_material_slot)
		{
			return i;
		}
	}

	return -1;
}

void impuls::asset_model::set_material(impuls::asset_ref<impuls::asset_material> in_material, const char* in_material_slot)
{
	const i32 mesh_idx = get_slot_index(in_material_slot);

	if (mesh_idx == -1)
		return;

	m_meshes[mesh_idx].second = in_material;
}

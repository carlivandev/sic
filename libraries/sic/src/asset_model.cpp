#include "sic/asset_model.h"
#include "sic/file_management.h"
#include "sic/system_asset.h"
#include "sic/asset_material.h"

#include "sic/core/defines.h"
#include "sic/core/logger.h"

#include "fmt/format.h"

#include <fstream>
#include <sstream>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace sic_private
{
	using namespace sic;

	Asset_model::Mesh process_mesh(aiMesh* in_mesh, const aiScene* in_scene)
	{
		Asset_model::Mesh new_mesh;
		new_mesh.m_positions.resize(in_mesh->mNumVertices);
		new_mesh.m_normals.resize(in_mesh->mNumVertices);
		new_mesh.m_texcoords.resize(in_mesh->mNumVertices);
		new_mesh.m_tangents.resize(in_mesh->mNumVertices);
		new_mesh.m_bitangents.resize(in_mesh->mNumVertices);

		for (ui16 i = 0; i < in_mesh->mNumVertices; i++)
		{
			glm::vec3 vert_pos;
			vert_pos.x = in_mesh->mVertices[i].x;
			vert_pos.y = in_mesh->mVertices[i].y;
			vert_pos.z = in_mesh->mVertices[i].z;
			new_mesh.m_positions[i] = vert_pos;

			glm::vec3 vert_normal;
			vert_normal.x = in_mesh->mNormals[i].x;
			vert_normal.y = in_mesh->mNormals[i].y;
			vert_normal.z = in_mesh->mNormals[i].z;
			new_mesh.m_normals[i] = vert_normal;

			if (in_mesh->mTextureCoords[0])
			{
				glm::vec2 vert_texture_coord;
				vert_texture_coord.x = in_mesh->mTextureCoords[0][i].x;
				vert_texture_coord.y = in_mesh->mTextureCoords[0][i].y;
				new_mesh.m_texcoords[i] = vert_texture_coord;
			}
			else
				new_mesh.m_texcoords[i] = glm::vec2(0.0f, 0.0f);
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
			new_mesh.m_material_slot = material->GetName().C_Str();
		}

		return std::move(new_mesh);
	}

	void process_node(aiNode* in_node, const aiScene* in_scene, std::vector<Asset_model::Mesh>& out_meshes)
	{
		// process all the node's meshes (if any)
		for (unsigned int i = 0; i < in_node->mNumMeshes; i++)
		{
			aiMesh* mesh = in_scene->mMeshes[in_node->mMeshes[i]];

			Asset_model::Mesh&& processed_mesh = process_mesh(mesh, in_scene);

			out_meshes.emplace_back(std::move(processed_mesh));
		}

		// then do the same for each of its children
		for (unsigned int i = 0; i < in_node->mNumChildren; i++)
			process_node(in_node->mChildren[i], in_scene, out_meshes);
	}
}

void sic::Asset_model::load(const State_assetsystem& in_assetsystem_state, Deserialize_stream& in_stream)
{
	size_t len = 0;
	in_stream.read(len);

	m_meshes.resize(len);
	m_materials.resize(len);

	for (ui32 i = 0; i < len; i++)
	{
		auto& mesh = m_meshes[i];

		in_stream.read(mesh.m_positions);
		in_stream.read(mesh.m_normals);
		in_stream.read(mesh.m_texcoords);
		in_stream.read(mesh.m_tangents);
		in_stream.read(mesh.m_bitangents);

		in_stream.read(mesh.m_indices);
		in_stream.read(mesh.m_material_slot);

		std::string header_id;
		in_stream.read(header_id);

		if (!header_id.empty())
			m_materials[i] = in_assetsystem_state.find_asset<Asset_material>(xg::Guid(header_id));
	}
}

void sic::Asset_model::save(const State_assetsystem& in_assetsystem_state, Serialize_stream& out_stream) const
{
	in_assetsystem_state;

	out_stream.write(m_meshes.size());

	for (ui32 i = 0; i < m_meshes.size(); i++)
	{
		const auto& mesh = m_meshes[i];
		const auto& mat = m_materials[i];

		out_stream.write(mesh.m_positions);
		out_stream.write(mesh.m_normals);
		out_stream.write(mesh.m_texcoords);
		out_stream.write(mesh.m_tangents);
		out_stream.write(mesh.m_bitangents);

		out_stream.write(mesh.m_indices);
		out_stream.write(mesh.m_material_slot);

		out_stream.write(mat.is_valid() ? mat.get_header()->m_id.str() : std::string(""));
	}
}

bool sic::Asset_model::import(std::string&& in_data)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFileFromMemory(in_data.data(), in_data.size(), aiProcess_Triangulate | aiProcess_FlipUVs);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		SIC_LOG_E(g_log_asset, "Assimp error: {0}", importer.GetErrorString());
		return false;
	}

	sic_private::process_node(scene->mRootNode, scene, m_meshes);

	if (m_meshes.empty())
		return false;

	for (ui32 i = 0; i < m_meshes.size(); i++)
		m_materials.emplace_back();

	return true;
}

void sic::Asset_model::get_dependencies(Asset_dependency_gatherer& out_dependencies)
{
	for (auto& mat_ref : m_materials)
		out_dependencies.add_dependency(*this, mat_ref);
}

const sic::Asset_ref<sic::Asset_material>& sic::Asset_model::get_material(i32 in_index) const
{
	return m_materials[in_index];
}

sic::i32 sic::Asset_model::get_slot_index(const char* in_material_slot) const
{
	const ui64 mesh_count = m_meshes.size();

	for (i32 i = 0; i < mesh_count; i++)
	{
		auto& mesh = m_meshes[i];
		if (mesh.m_material_slot == in_material_slot)
		{
			return i;
		}
	}

	return -1;
}

void sic::Asset_model::set_material(const sic::Asset_ref<sic::Asset_material>& in_material, const char* in_material_slot)
{
	const i32 mesh_idx = get_slot_index(in_material_slot);

	if (mesh_idx == -1)
		return;

	m_materials[mesh_idx] = in_material;
}

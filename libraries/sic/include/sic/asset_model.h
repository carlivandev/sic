#pragma once
#include "sic/asset.h"
#include "sic/gl_includes.h"
#include "sic/opengl_vertex_buffer_array.h"

#include "sic/core/defines.h"
#include "sic/core/type_restrictions.h"

#include <string>
#include <vector>
#include <memory>
#include <optional>

#include "glm/vec3.hpp"
#include "glm/vec2.hpp"

namespace sic
{
	struct Asset_material;

	struct Asset_model : Asset
	{
		struct Mesh : Noncopyable
		{
			Mesh() = default;
			Mesh(Mesh&& in_other) = default;
			Mesh& operator=(Mesh&& in_other) = default;

			std::vector<glm::vec3> m_positions;
			std::vector<glm::vec3> m_normals;
			std::vector<glm::vec2> m_texcoords;
			std::vector<glm::vec3> m_tangents;
			std::vector<glm::vec3> m_bitangents;

			std::vector<ui32> m_indices;
			std::string m_material_slot;

			std::optional<OpenGl_vertex_buffer_array
			<
				OpenGl_vertex_attribute_position3D,
				OpenGl_vertex_attribute_normal,
				OpenGl_vertex_attribute_texcoord,
				OpenGl_vertex_attribute_tangent,
				OpenGl_vertex_attribute_bitangent
			>> m_vertex_buffer_array;

			std::optional<OpenGl_buffer> m_index_buffer;
		};

		bool has_post_load() override final { return true; }
		bool has_pre_unload() override final { return true; }

		void load(const State_assetsystem& in_assetsystem_state, Deserialize_stream& in_stream) override;
		void save(const State_assetsystem& in_assetsystem_state, Serialize_stream& out_stream) const override;

		void get_dependencies(Asset_dependency_gatherer& out_dependencies) override;

		const Asset_ref<Asset_material>& get_material(i32 in_index) const;

		//returns -1 if invalid slot
		i32 get_slot_index(const char* in_material_slot) const;

		void set_material(const Asset_ref<Asset_material>& in_material, const char* in_material_slot);


		std::vector<Mesh> m_meshes;
		std::vector<Asset_ref<Asset_material>> m_materials;
	};
}
#pragma once
#include "impuls/asset.h"
#include "impuls/defines.h"
#include "impuls/gl_includes.h"

#include <string>
#include <vector>
#include <memory>

#include "glm/vec3.hpp"
#include "glm/vec2.hpp"

namespace impuls
{
	struct asset_material;

	struct asset_model : i_asset
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

		bool has_post_load() override final { return true; }
		bool has_pre_unload() override final { return true; }

		void load(const state_assetsystem& in_assetsystem_state, deserialize_stream& in_stream) override;
		void save(const state_assetsystem& in_assetsystem_state, serialize_stream& out_stream) const override;
		bool import(std::string&& in_data);

		asset_ref<asset_material> get_material(i32 in_index) const;

		//returns -1 if invalid slot
		i32 get_slot_index(const char* in_material_slot) const;

		void set_material(asset_ref<asset_material> in_material, const char* in_material_slot);


		std::vector<std::pair<mesh, asset_ref<asset_material>>> m_meshes;
	};
}
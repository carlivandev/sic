#pragma once
#include "impuls/world_context.h"
#include "impuls/render_object_list.h"

#include "impuls/asset.h"

#include "glm/vec3.hpp"

namespace impuls
{
	struct asset_model;
	struct asset_material;

	struct render_object_model
	{
		asset_ref<asset_model> m_model;
		std::unordered_map<std::string, asset_ref<asset_material>> m_material_overrides;
		glm::vec3 m_position = { 0.0f, 0.0f, 0.0f };
	};

	enum class e_debug_shape
	{
		box,
		sphere,
		none
	};

	struct render_object_debug_shape
	{
		e_debug_shape m_shape = e_debug_shape::box;
		//use life time to push to destroy post render if < 0
		float m_lifetime = -1.0f;
	};

	struct state_render_scene : i_state
	{
		friend struct system_renderer;

		render_object_list<render_object_model> m_models;
		render_object_list<render_object_debug_shape> m_debug_shapes;

	protected:
		void flush_updates();
	};
}
#pragma once
#include "system.h"
#include "world_context.h"

#include "impuls/system_asset.h"
#include "impuls/double_buffer.h"

#include "glm/vec3.hpp"

namespace impuls
{
	struct asset_model;
	struct asset_material;

	struct drawcall_model
	{
		asset_ref<asset_model> m_model;
		std::unordered_map<std::string, asset_ref<asset_material>> m_material_overrides;
		glm::vec3 m_position = { 0.0f, 0.0f, 0.0f };
	};

	struct state_renderer : public i_state
	{
		double_buffer<std::vector<drawcall_model>> m_model_drawcalls;
	};

	struct system_renderer : i_system
	{
		virtual void on_created(world_context&& in_context) const override;
		virtual void on_tick(world_context&& in_context, float in_time_delta) const override;
	};
}
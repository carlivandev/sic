#pragma once
#include "system.h"
#include "world_context.h"

#include "system_asset.h"

namespace impuls
{
	struct asset_model;
	struct asset_material;
	struct component_transform;

	struct component_model : public i_component
	{
		friend struct system_model;

	public:
		void set_material(asset_ref<asset_material> in_material, const char* in_material_slot);
		asset_ref<asset_material> get_material_override(const char* in_material_slot) const;

	public:
		component_transform* m_transform = nullptr;
		asset_ref<asset_model> m_model;

	protected:
		std::unordered_map<std::string, asset_ref<asset_material>> m_material_overrides;
	};

	struct system_model : i_system
	{
		virtual void on_created(world_context&& in_context) const override;
		virtual void on_tick(world_context&& in_context, float in_time_delta) const override;
	};
}
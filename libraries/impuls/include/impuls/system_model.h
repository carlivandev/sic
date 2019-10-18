#pragma once
#include "system.h"
#include "world_context.h"

namespace impuls
{
	struct asset_model;
	struct asset_material;
	struct component_transform;

	struct component_model : public i_component
	{
	public:
		void set_material(std::shared_ptr<asset_material>& in_material, const char* in_material_slot);
		std::shared_ptr<asset_material> get_material(i32 in_index);

	public:
		std::shared_ptr<asset_model> m_model;
		component_transform* m_transform = nullptr;

	protected:
		std::vector<std::shared_ptr<asset_material>> m_material_overrides;
	};

	struct system_model : i_system
	{
		virtual void on_created(world_context&& in_context) const override;
		virtual void on_tick(world_context&& in_context, float in_time_delta) const override;
	};
}
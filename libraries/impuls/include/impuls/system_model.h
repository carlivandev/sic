#pragma once
#include "impuls/system.h"
#include "impuls/engine_context.h"
#include "impuls/system_asset.h"
#include "impuls/component_transform.h"

namespace impuls
{
	struct asset_model;
	struct asset_material;
	struct state_render_scene;

	struct component_model : public i_component
	{
		friend struct system_model;

	public:
		void set_model(const asset_ref<asset_model>& in_model);

		void set_material(asset_ref<asset_material> in_material, const std::string& in_material_slot);
		asset_ref<asset_material> get_material_override(const std::string& in_material_slot) const;

	protected:
		state_render_scene* m_render_scene_state = nullptr;
		asset_ref<asset_model> m_model;

		std::unordered_map<std::string, asset_ref<asset_material>> m_material_overrides;
		i32 m_render_object_id = -1;

		component_transform::on_updated::handle m_on_updated_handle;
	};

	struct system_model : i_system
	{
		virtual void on_created(engine_context&& in_context) override;
	};
}
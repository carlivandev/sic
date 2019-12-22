#pragma once
#include "sic/system.h"
#include "sic/engine_context.h"
#include "sic/system_asset.h"
#include "sic/component_transform.h"
#include "sic/state_render_scene.h"

namespace sic
{
	struct Asset_model;
	struct Asset_material;
	struct State_render_scene;

	struct Component_model : public Component
	{
		friend struct System_model;

	public:
		void set_model(const Asset_ref<Asset_model>& in_model);

		void set_material(Asset_ref<Asset_material> in_material, const std::string& in_material_slot);
		Asset_ref<Asset_material> get_material_override(const std::string& in_material_slot) const;

	protected:
		State_render_scene* m_render_scene_state = nullptr;
		Asset_ref<Asset_model> m_model;

		std::unordered_map<std::string, Asset_ref<Asset_material>> m_material_overrides;
		Update_list_id<Render_object_model> m_render_object_id;

		Component_transform::On_updated::Handle m_on_updated_handle;
	};

	struct System_model : System
	{
		virtual void on_created(Engine_context&& in_context) override;
	};
}
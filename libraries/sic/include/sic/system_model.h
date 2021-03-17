#pragma once
#include "sic/system_asset.h"
#include "sic/component_transform.h"
#include "sic/state_render_scene.h"

#include "sic/core/system.h"
#include "sic/core/engine_context.h"
#include "sic/core/async_result.h"

namespace sic
{
	struct Asset_model;
	struct Asset_material;
	struct State_render_scene;

	struct State_model : State
	{
		i32 generate_id()
		{
			if (m_free_ids.empty())
				return m_id_ticker++;

			const i32 ret = m_free_ids.back();
			m_free_ids.pop_back();
			return ret;
		}

		void return_id(i32 in_id)
		{
			m_free_ids.push_back(in_id);
		}

	private:
		i32 m_id_ticker = 0;
		std::vector<i32> m_free_ids;
	};

	using Processor_model = Engine_processor<Processor_flag_read_single<Component_transform>, Processor_flag_write<State_model>>;

	struct Component_model : public Component
	{
		friend struct System_model;
		friend struct Component_dynamic_material_parameters;

	public:
		Async_query set_model(Processor_model in_processor, const Asset_ref<Asset_model>& in_model);

		void set_material(Processor_model in_processor, Asset_ref<Asset_material> in_material, const std::string& in_material_slot);

		Asset_ref<Asset_material> get_material_override(const std::string& in_material_slot) const;

	protected:
		void try_destroy_render_object(Processor_model in_processor);
		void try_create_render_object(Processor_model in_processor);

		Asset_ref<Asset_model> m_model;

		std::unordered_map<std::string, Asset_ref<Asset_material>> m_material_overrides;
		std::vector<Render_object_id<Render_object_mesh>> m_render_object_id_collection;

		Component_transform::On_updated::Handle m_on_updated_handle;
		std::vector<std::function<void(Engine_processor<Processor_flag_write<State_render_scene>>)>> m_pending_updates;
	};

	

	struct System_model : System
	{
		virtual void on_created(Engine_context in_context) override;
	};
}
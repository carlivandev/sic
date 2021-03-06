#pragma once
#include "sic/asset_material.h"
#include "sic/state_render_scene.h"

#include "sic/core/scene_context.h"
#include "sic/core/weak_ref.h"
#include "sic/core/async_result.h"

#include "glm/vec4.hpp"

namespace sic
{
	struct Component_model;
	struct Component_dynamic_material_parameters;

	using Material_dynamic_parameter_processor = Engine_processor<Processor_flag_read_single<Component_model>, Processor_flag_write_single<Component_dynamic_material_parameters>>;

	struct Component_dynamic_material_parameters : Component
	{
		void bind(Material_dynamic_parameter_processor in_processor, Component_model& inout_model_component, std::optional<std::string> in_slot = {});

		Async_query set_parameter(Material_dynamic_parameter_processor in_processor, const std::string& in_name, Asset_ref<Asset_texture_base> in_value);

		Async_query set_parameter(Material_dynamic_parameter_processor in_processor, const std::string& in_name, const glm::vec4& in_value);

		void unbind(Material_dynamic_parameter_processor in_processor);

		void reapply_all_parameters(Material_dynamic_parameter_processor in_processor);

		Weak_ref<Component_model> get_model() const { return m_weak_model; }

	private:
		template <typename T>
		std::optional<Async_query> pre_set_model_check(Material_dynamic_parameter_processor in_processor, const std::string& in_name, const T& in_value);

		template <typename T>
		void set_parameter_internal(Engine_processor<> in_processor, const std::string& in_name, const T& in_value);

		Weak_ref<Component_model> m_weak_model;
		//if not set, apply parameters to all materials
		std::optional<std::string> m_slot;
		std::unordered_map<std::string, Asset_ref<Asset_texture_base>> m_texture_parameters;
		std::unordered_map<std::string, glm::vec4> m_vec4_parameters;
		On_asset_loaded::Handle m_model_loaded_handle;
	};
	template<typename T>
	inline std::optional<Async_query> Component_dynamic_material_parameters::pre_set_model_check(Material_dynamic_parameter_processor in_processor, const std::string& in_name, const T& in_value)
	{
		const Component_model* model = m_weak_model.try_get_r(in_processor);

		if (!model)
			return {};

		if (!model->m_model.is_valid())
			return {};

		Async_query query;

		if (model->m_model.get_load_state() != Asset_load_state::loaded)
		{
			query.setup();

			auto loaded_cb = [weak_this = Weak_ref(this), in_name, in_value, in_processor, query] (Asset_loaded_processor in_processor, Asset*)
			{
				in_processor.schedule
				(
					[weak_this, in_name, in_value, query](Engine_processor<Processor_model, Processor_flag_write_single<Component_model>, Material_dynamic_parameter_processor> in_processor) mutable
					{
						query.trigger();

						Component_dynamic_material_parameters* this_ptr = weak_this.try_get_w(in_processor);
						if (!this_ptr)
							return;

						this_ptr->set_parameter(in_processor, in_name, in_value);
					}
				);
			};

			m_model_loaded_handle.set_callback(loaded_cb);
			model->m_model.get_header()->m_on_loaded_delegate.try_bind(m_model_loaded_handle);
			return query;
		}

		return query;
	}

	template<typename T>
	inline void Component_dynamic_material_parameters::set_parameter_internal(Engine_processor<> in_processor, const std::string& in_name, const T& in_value)
	{
		in_processor.schedule
		(
			[weak_this = Weak_ref(this), in_name, in_value](Engine_processor<Processor_flag_write<State_render_scene>, Processor_flag_read_single<Component_dynamic_material_parameters>, Processor_flag_read_single<Component_model>> in_processor_render_scene)
			{
				const Component_dynamic_material_parameters* this_ptr = weak_this.try_get_r(in_processor_render_scene);
				if (!this_ptr)
					return;

				const Component_model* model = this_ptr->m_weak_model.try_get_r(in_processor_render_scene);
				if (!model)
					return;

				const Asset_model* loaded_model = model->m_model.get();

				auto update_ro_cb =
				[&state = in_processor_render_scene.get_state_checked_w<State_render_scene>()](const Render_object_id<Render_object_mesh> in_ro_id, const std::string& in_param_name, const T& in_value_to_set) mutable
				{
					state.update_object<Render_object_mesh>
					(
						in_ro_id,
						[param_name = in_param_name, value = in_value_to_set](Render_object_mesh& in_out_object)
						{
							in_out_object.m_material->set_parameter_on_instance(param_name.c_str(), value, in_out_object.m_instance_data_index);
						}
					);
				};

				if (this_ptr->m_slot.has_value())
				{
					const i32 slot_idx = loaded_model->get_slot_index(this_ptr->m_slot.value().c_str());
					auto& ro_id = model->m_render_object_id_collection[slot_idx];

					update_ro_cb(ro_id, in_name, in_value);
				}
				else
				{
					for (auto& ro_id : model->m_render_object_id_collection)
						update_ro_cb(ro_id, in_name, in_value);
				}
			}
		);

	}
}
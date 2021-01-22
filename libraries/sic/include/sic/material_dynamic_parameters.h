#pragma once
#include "sic/asset_material.h"
#include "sic/state_render_scene.h"

#include "sic/core/scene_context.h"
#include "sic/core/weak_ref.h"

#include "glm/vec4.hpp"

namespace sic
{
	struct Component_model;

	struct Material_dynamic_parameters : Noncopyable
	{
		void bind(Processor<> in_processor, Component_model& inout_model_component, std::optional<std::string> in_slot = {});

		void set_parameter(Processor<> in_processor, const std::string& in_name, Asset_ref<Asset_texture_base> in_value);

		void set_parameter(Processor<> in_processor, const std::string& in_name, const glm::vec4& in_value);

		void unbind(Processor<> in_processor);

	private:
		template <typename T>
		bool pre_set_model_check(Processor<> in_processor, const std::string& in_name, const T& in_value);

		template <typename T>
		void set_parameter_internal(Processor<> in_processor, const std::string& in_name, const T& in_value);

		Weak_ref<Component_model> m_weak_model;
		//if not set, apply parameters to all materials
		std::optional<std::string> m_slot;
		std::unordered_map<std::string, Asset_ref<Asset_texture_base>> m_texture_parameters;
		std::unordered_map<std::string, glm::vec4> m_vec4_parameters;
		std::unordered_map<Asset_material*, On_material_modified::Handle> m_on_material_modified_handles;
		On_asset_loaded::Handle m_model_loaded_handle;
	};
	template<typename T>
	inline bool Material_dynamic_parameters::pre_set_model_check(Processor<> in_processor, const std::string& in_name, const T& in_value)
	{
		const Component_model* model = m_weak_model.try_get(Scene_context(*in_processor.m_engine, *in_processor.m_scene));

		if (!model)
			return false;

		if (!model->m_model.is_valid())
			return false;

		if (model->m_model.get_load_state() != Asset_load_state::loaded)
		{
			auto loaded_cb = [this, in_name, in_value, in_processor](Asset*) { set_parameter(in_processor, in_name, in_value); };
			m_model_loaded_handle.set_callback(loaded_cb);
			model->m_model.get_header()->m_on_loaded_delegate.try_bind(m_model_loaded_handle);
			return false;
		}

		return true;
	}

	template<typename T>
	inline void Material_dynamic_parameters::set_parameter_internal(Processor<> in_processor, const std::string& in_name, const T& in_value)
	{
		in_processor.schedule
		(
			std::function([this, in_name, in_value](Processor<Processor_flag_write<State_render_scene>> in_processor_render_scene)
			{
				const Component_model* model = m_weak_model.try_get(Scene_context(*in_processor_render_scene.m_engine, *in_processor_render_scene.m_scene));
				const Asset_model* loaded_model = model->m_model.get();

				auto update_ro_cb =
				[this, model, &state = in_processor_render_scene.get_state_checked_w<State_render_scene>()](const Render_object_id<Render_object_mesh> in_ro_id, const std::string& in_param_name, const T& in_value_to_set) mutable
				{
					state.update_object<Render_object_mesh>
					(
						in_ro_id,
						[param_name = in_param_name, value = in_value_to_set, this](Render_object_mesh& in_out_object)
						{
							auto& handle = m_on_material_modified_handles[in_out_object.m_material];
							handle.set_callback
							(
								[this, data_idx = in_out_object.m_instance_data_index](Asset_material* in_material)
								{
									for (auto& param : m_texture_parameters)
										in_material->set_parameter_on_instance(param.first.c_str(), param.second, data_idx);

									for (auto& param : m_vec4_parameters)
										in_material->set_parameter_on_instance(param.first.c_str(), param.second, data_idx);
								}
							);

							in_out_object.m_material->m_on_modified_delegate.try_bind(handle);
							in_out_object.m_material->set_parameter_on_instance(param_name.c_str(), value, in_out_object.m_instance_data_index);
						}
					);
				};

				if (m_slot.has_value())
				{
					const i32 slot_idx = loaded_model->get_slot_index(m_slot.value().c_str());
					auto& ro_id = model->m_render_object_id_collection[slot_idx];

					update_ro_cb(ro_id, in_name, in_value);
				}
				else
				{
					for (auto& ro_id : model->m_render_object_id_collection)
						update_ro_cb(ro_id, in_name, in_value);
				}
			})
		);

	}
}
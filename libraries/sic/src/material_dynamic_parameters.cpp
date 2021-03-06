#pragma once
#include "sic/material_dynamic_parameters.h"
#include "sic/system_model.h"

void sic::Component_dynamic_material_parameters::bind(Material_dynamic_parameter_processor in_processor, Component_model& inout_model_component, std::optional<std::string> in_slot)
{
	const Component_model* model = m_weak_model.try_get_r(in_processor);

	if (model != &inout_model_component)
		unbind(in_processor);

	m_slot = in_slot;
	m_weak_model.set(&inout_model_component);
}

sic::Async_query sic::Component_dynamic_material_parameters::set_parameter(Material_dynamic_parameter_processor in_processor, const std::string& in_name, Asset_ref<Asset_texture_base> in_value)
{
	auto query = pre_set_model_check(in_processor, in_name, in_value);
	if (!query.has_value())
		return Async_query();

	auto& texture = m_texture_parameters[in_name] = in_value;

	if (texture.is_valid() && texture.get_load_state() != Asset_load_state::loaded)
	{
		Async_query texture_query;
		texture_query.setup();

		query.value().then
		(
			in_processor,
			[weak_this = Weak_ref(this), texture_query, in_name, texture](Ep<Material_dynamic_parameter_processor> in_processor) mutable
			{
				auto* this_ptr = weak_this.try_get_w(in_processor);
				if (!this_ptr)
				{
					texture_query.trigger();
					return;
				}

				auto loaded_cb = [weak_this, in_name, texture, texture_query](Asset_loaded_processor in_processor, Asset*) mutable
				{
					in_processor.schedule([weak_this, in_name, texture, texture_query](Material_dynamic_parameter_processor in_processor) mutable
					{
						if (auto* this_ptr = weak_this.try_get_w(in_processor))
							this_ptr->set_parameter(in_processor, in_name, texture);

						texture_query.trigger();
					});
				};

				this_ptr->m_texture_parameters[in_name].bind_on_loaded(loaded_cb);
			}
		);

		return texture_query;
	}

	set_parameter_internal(in_processor, in_name, in_value);

	return query.value();
}

sic::Async_query sic::Component_dynamic_material_parameters::set_parameter(Material_dynamic_parameter_processor in_processor, const std::string& in_name, const glm::vec4& in_value)
{
	auto query = pre_set_model_check(in_processor, in_name, in_value);
	if (!query.has_value())
		return Async_query();

	auto& value = m_vec4_parameters[in_name] = in_value;
	set_parameter_internal(in_processor, in_name, value);

	return query.value();
}

void sic::Component_dynamic_material_parameters::unbind(Material_dynamic_parameter_processor in_processor)
{
	const Component_model* model = m_weak_model.try_get_r(in_processor);

	if (model && model->m_model.is_valid())
		model->m_model.get_header()->m_on_loaded_delegate.unbind(m_model_loaded_handle);

	m_slot.reset();
	m_texture_parameters.clear();
	m_vec4_parameters.clear();
}

void sic::Component_dynamic_material_parameters::reapply_all_parameters(Material_dynamic_parameter_processor in_processor)
{
	for (auto& param : m_texture_parameters)
		set_parameter(in_processor, param.first, param.second);

	for (auto& param : m_vec4_parameters)
		set_parameter(in_processor, param.first, param.second);
}
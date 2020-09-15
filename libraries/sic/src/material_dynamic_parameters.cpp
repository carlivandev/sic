#pragma once
#include "sic/material_dynamic_parameters.h"
#include "sic/system_model.h"

void sic::Material_dynamic_parameters::bind(Processor_render_scene_update in_processor, Component_model& inout_model_component, std::optional<std::string> in_slot)
{
	const Component_model* model = m_weak_model.try_get(Scene_context(*in_processor.m_engine, *in_processor.m_scene));

	if (model != &inout_model_component)
		unbind(in_processor);

	m_slot = in_slot;
	m_weak_model.set(&inout_model_component);
}

void sic::Material_dynamic_parameters::set_parameter(Processor_render_scene_update in_processor, const std::string& in_name, Asset_ref<Asset_texture_base> in_value)
{
	if (!pre_set_model_check(in_processor, in_name, in_value))
		return;

	auto& texture = m_texture_parameters[in_name] = in_value;

	if (texture.is_valid() && texture.get_load_state() != Asset_load_state::loaded)
	{
		auto loaded_cb = [this, in_name, texture, in_processor](Asset*) { set_parameter(in_processor, in_name, texture); };
		texture.bind_on_loaded(loaded_cb);
		return;
	}

	set_parameter_internal(in_processor, in_name, in_value);
}

void sic::Material_dynamic_parameters::set_parameter(Processor_render_scene_update in_processor, const std::string& in_name, const glm::vec4& in_value)
{
	if (!pre_set_model_check(in_processor, in_name, in_value))
		return;

	auto& value = m_vec4_parameters[in_name] = in_value;
	set_parameter_internal(in_processor, in_name, value);
}

void sic::Material_dynamic_parameters::unbind(Processor_render_scene_update in_processor)
{
	const Component_model* model = m_weak_model.try_get(Scene_context(*in_processor.m_engine, *in_processor.m_scene));

	if (model && model->m_model.is_valid())
		model->m_model.get_header()->m_on_loaded_delegate.unbind(m_model_loaded_handle);

	m_slot.reset();
	m_texture_parameters.clear();
	m_vec4_parameters.clear();
}
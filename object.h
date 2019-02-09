#pragma once

namespace impuls
{
	struct i_component;

	struct component_handle
	{
		component_handle(ui32 in_type_index, i_component* in_component) : m_type_index(in_type_index), m_component(in_component) {}
		ui32 m_type_index;
		i_component* m_component;
	};
	struct object
	{
		bool is_valid() const { return m_instance_index != -1; }
		std::vector<component_handle> m_components;
		i32 m_instance_index = -1;
	};
}
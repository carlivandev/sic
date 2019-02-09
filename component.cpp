#include "stdafx.h"
#include "component.h"

namespace impuls
{
	ui32 component_storage::index_ticker = 0;

	inline i_component* impuls::component_storage::next_valid_component(i32 in_instance_index)
	{
		for (ui32 i = in_instance_index + 1; i < m_components.size() / m_component_type_size; i++)
		{
			i_component* comp = reinterpret_cast<i_component*>(&m_components[i * m_component_type_size]);

			if (comp->is_valid())
				return comp;
		}

		return nullptr;
	}

	i_component* component_storage::previous_valid_component(i32 in_instance_index)
	{
		for (i32 i = in_instance_index - 1; i >= 0; i--)
		{
			i_component* comp = reinterpret_cast<i_component*>(&m_components[i * m_component_type_size]);

			if (comp->is_valid())
				return comp;
		}

		return nullptr;
	}
}

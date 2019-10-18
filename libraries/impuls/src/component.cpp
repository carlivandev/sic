#include "impuls/pch.h"
#include "impuls/component.h"

namespace impuls
{
	void component_storage::initialize_with_typesize(ui32 in_initial_capacity, ui32 in_bucket_capacity, ui32 in_typesize)
	{
		m_component_type_size = in_typesize;
		m_bucket_size = in_bucket_capacity;

		m_components.allocate(in_initial_capacity * m_component_type_size, in_bucket_capacity * m_component_type_size, m_component_type_size);
	}

	inline i_component_base* impuls::component_storage::next_valid_component(i32 in_instance_index)
	{
		for (ui32 i = in_instance_index + 1; i < m_components.size() / m_component_type_size; i++)
		{
			i_component_base* comp = reinterpret_cast<i_component_base*>(&m_components[i * m_component_type_size]);

			if (comp->is_valid())
				return comp;
		}

		return nullptr;
	}

	i_component_base* component_storage::previous_valid_component(i32 in_instance_index)
	{
		for (i32 i = in_instance_index - 1; i >= 0; i--)
		{
			i_component_base* comp = reinterpret_cast<i_component_base*>(&m_components[i * m_component_type_size]);

			if (comp->is_valid())
				return comp;
		}

		return nullptr;
	}
}

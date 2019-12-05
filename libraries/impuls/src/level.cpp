#include "impuls/pch.h"
#include "impuls/level.h"

namespace impuls
{
	std::unique_ptr<i_component_storage>& level::get_component_storage_at_index(i32 in_index)
	{
		return m_component_storages[in_index];
	}

	std::unique_ptr<i_object_storage_base>& level::get_object_storage_at_index(i32 in_index)
	{
		return m_objects[in_index];
	}
}
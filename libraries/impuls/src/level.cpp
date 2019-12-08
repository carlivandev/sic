#include "impuls/pch.h"
#include "impuls/level.h"
#include "impuls/object.h"

namespace impuls
{
	void level::destroy_object(i_object_base& in_object_to_destroy)
	{
		const ui32 type_idx = in_object_to_destroy.m_type_index;
		assert((type_idx < m_objects.size() || m_objects[type_idx].get() != nullptr) && "type not registered");

		//unbind from parent if any
		in_object_to_destroy.unchild();

		//destroy children recursive
		const i16 last_idx = in_object_to_destroy.m_children.size() - 1;
		for (i16 i = last_idx; i >= 0; i--)
			destroy_object(*in_object_to_destroy.m_children[i]);

		//destroy in_obj
		auto* storage_to_destroy_from = reinterpret_cast<object_storage*>(m_objects[type_idx].get());

		level_context context(m_engine, *this);
		storage_to_destroy_from->destroy_instance(context, in_object_to_destroy);
	}

	std::unique_ptr<i_component_storage>& level::get_component_storage_at_index(i32 in_index)
	{
		return m_component_storages[in_index];
	}

	std::unique_ptr<i_object_storage_base>& level::get_object_storage_at_index(i32 in_index)
	{
		return m_objects[in_index];
	}
}
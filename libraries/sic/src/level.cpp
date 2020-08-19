#include "sic/level.h"
#include "sic/object.h"

namespace sic
{
	void Scene::destroy_object(Object_base& in_object_to_destroy)
	{
		const ui32 type_idx = in_object_to_destroy.m_type_index;
		assert((type_idx < m_objects.size() || m_objects[type_idx].get() != nullptr) && "type not registered");

		//unbind from parent if any
		in_object_to_destroy.unchild();

		//destroy children recursive
		const i16 last_idx = static_cast<i16>(in_object_to_destroy.m_children.size()) - 1;
		for (i16 i = last_idx; i >= 0; i--)
			destroy_object(*in_object_to_destroy.m_children[i]);

		//destroy in_obj
		auto* storage_to_destroy_from = reinterpret_cast<Object_storage*>(m_objects[type_idx].get());

		Scene_context context(m_engine, *this);
		storage_to_destroy_from->destroy_instance(context, in_object_to_destroy);

		{
			std::scoped_lock lock(m_object_creation_mutex);
			m_object_exists_flags[in_object_to_destroy.m_id] = false;
		}
	}

	bool Scene::get_does_object_exist(Object_id in_id, bool in_only_in_this_level) const
	{
		if (!in_id.get_is_set())
			return false;

		const i32 id = in_id.get_id();

		if (in_only_in_this_level)
		{
			std::scoped_lock lock(m_object_creation_mutex);
			return m_object_exists_flags.size() > id && m_object_exists_flags[id];
		}
		
		if (m_outermost_level)
			return m_outermost_level->get_does_object_exist(in_id, false);

		std::scoped_lock lock(m_object_creation_mutex);

		if (m_object_exists_flags.size() > id && m_object_exists_flags[id])
			return true;

		for (auto&& sublevel : m_sublevels)
		{
			if (sublevel->get_does_object_exist(in_id, false))
				return true;
		}

		return false;
	}

	std::unique_ptr<Component_storage_base>& Scene::get_component_storage_at_index(i32 in_index)
	{
		return m_component_storages[in_index];
	}

	std::unique_ptr<Object_storage_base>& Scene::get_object_storage_at_index(i32 in_index)
	{
		return m_objects[in_index];
	}
}
#pragma once
#include "sic/component.h"
#include "sic/object_base.h"
#include "sic/level_context.h"

#include <type_traits>

namespace sic
{
	template <typename T>
	struct Weak_ref
	{
		Weak_ref() = default;
		Weak_ref(T* inout_object_or_component) { set(inout_object_or_component); }

		void set(T* inout_object_or_component);

		T* try_get(const Scene_context& in_context) { if (m_object_or_component && in_context.get_does_object_exist(m_id, false)) return m_object_or_component; else return nullptr; }
		const T* try_get(const Scene_context& in_context) const { if (m_object_or_component && in_context.get_does_object_exist(m_id, false)) return m_object_or_component;  else return nullptr; }

	private:
		T* m_object_or_component = nullptr;
		Object_id m_id;
	};
	template<typename T>
	inline void Weak_ref<T>::set(T* inout_object_or_component)
	{
		m_object_or_component = inout_object_or_component;

		if (m_object_or_component == nullptr)
		{
			m_id = Object_id();
			return;
		}

		if constexpr (std::is_base_of<Component_base, T>::value)
		{
			m_id = m_object_or_component->get_owner().get_id();
		}
		else if constexpr (std::is_base_of<Object_base, T>::value)
		{
			m_id = m_object_or_component->get_id();
		}
		else
		{
			static_assert(false, "T must be either an object or a component.");
		}
	}
}
#pragma once
#include <iterator>

#include "component.h"

namespace impuls
{
	template <typename component_type, typename storage_type>
	struct component_iterator : public std::iterator<std::input_iterator_tag, component_type>
	{
		component_iterator() = default;
		component_iterator(storage_type* in_storage, i32 in_index) : m_storage(in_storage), m_index(in_index)
		{
			try_find_next_valid_index();
		}

		component_iterator& operator++()
		{
			++m_index;
			try_find_next_valid_index();

			return *this;
		}
		bool operator == (const component_iterator<component_type, storage_type>& rhs) const { return m_index == rhs.m_index; }
		bool operator != (const component_iterator<component_type, storage_type>& rhs) const { return m_index != rhs.m_index; }
		component_type& operator* () { return *reinterpret_cast<component_type*>(&m_storage->m_components[m_index * m_storage->m_component_type_size]); }
		const component_type& operator* () const { return *reinterpret_cast<component_type*>(&m_storage->m_components[m_index * m_storage->m_component_type_size]); }

		void try_find_next_valid_index()
		{
			while (m_index < static_cast<int>(m_storage->m_components.size() / m_storage->m_component_type_size) &&
				!reinterpret_cast<component_type*>(&m_storage->m_components[m_index * m_storage->m_component_type_size])->is_valid())
			{
				++m_index;
			}
		}

		storage_type* m_storage = nullptr;
		i32 m_index = -1;
	};

	template <typename component_type, typename storage_type>
	struct component_view
	{
		typedef component_iterator<component_type, storage_type> iterator;
		typedef component_iterator<const component_type, const storage_type> const_iterator;

		component_view() = default;
		component_view(storage_type* in_storage) : m_storage(in_storage) {}

		iterator begin() { return iterator(m_storage, 0); }
		iterator end() { return iterator(m_storage, static_cast<i32>(m_storage->m_components.size() / m_storage->m_component_type_size)); }
		const const_iterator begin() const { return const_iterator(m_storage, 0); }
		const const_iterator end() const { return const_iterator(m_storage, static_cast<i32>(m_storage->m_components.size() / m_storage->m_component_type_size)); }

		storage_type* m_storage = nullptr;
	};
}
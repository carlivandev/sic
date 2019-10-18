#pragma once
#include <iterator>
#include "bucket_allocator.h"

namespace impuls
{
	template <typename t_type>
	struct bucket_allocator_iterator : public std::iterator<std::input_iterator_tag, t_type>
	{
		bucket_allocator_iterator() = default;
		bucket_allocator_iterator(bucket_byte_allocator* in_storage, i32 in_index) : m_storage(in_storage), m_index(in_index)
		{
			try_find_next_valid_index();
		}

		bucket_allocator_iterator& operator++()
		{
			++m_index;
			try_find_next_valid_index();

			return *this;
		}
		bool operator == (const bucket_allocator_iterator<t_type>& rhs) const { return m_index == rhs.m_index; }
		bool operator != (const bucket_allocator_iterator<t_type>& rhs) const { return m_index != rhs.m_index; }
		t_type& operator* () { return *reinterpret_cast<t_type*>(&m_storage->operator[](m_index * m_storage->m_typesize)); }
		const t_type& operator* () const { return *reinterpret_cast<t_type*>(&m_storage->operator[](m_index * m_storage->m_typesize)); }

		void try_find_next_valid_index()
		{
			while (m_index < static_cast<int>(m_storage->size() / m_storage->m_typesize) &&
				!reinterpret_cast<t_type*>(&(*m_storage)[m_index * m_storage->m_typesize])->is_valid())
			{
				++m_index;
			}
		}

		bucket_byte_allocator* m_storage = nullptr;
		i32 m_index = -1;
	};

	template <typename t_type>
	struct bucket_allocator_view
	{
		typedef bucket_allocator_iterator<t_type> iterator;
		typedef bucket_allocator_iterator<const t_type> const_iterator;

		bucket_allocator_view() = default;
		bucket_allocator_view(bucket_byte_allocator* in_storage) : m_storage(in_storage) {}

		iterator begin() { return iterator(m_storage, 0); }
		iterator end() { return iterator(m_storage, static_cast<i32>(m_storage->size() / m_storage->m_typesize)); }
		const const_iterator begin() const { return const_iterator(m_storage, 0); }
		const const_iterator end() const { return const_iterator(m_storage, static_cast<i32>(m_storage->size() / m_storage->m_typesize)); }

		bucket_byte_allocator* m_storage = nullptr;
	};
}
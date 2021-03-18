#pragma once
#include <iterator>
#include "bucket_allocator.h"

namespace sic
{
	template <typename T_type>
	struct Bucket_allocator_iterator
	{
		Bucket_allocator_iterator() = default;
		Bucket_allocator_iterator(Bucket_byte_allocator* in_storage, i32 in_index) : m_storage(in_storage), m_index(in_index)
		{
			try_find_next_valid_index();
		}

		Bucket_allocator_iterator& operator++()
		{
			++m_index;
			try_find_next_valid_index();

			return *this;
		}
		bool operator == (const Bucket_allocator_iterator<T_type>& rhs) const { return m_index == rhs.m_index; }
		bool operator != (const Bucket_allocator_iterator<T_type>& rhs) const { return m_index != rhs.m_index; }
		T_type& operator* () { return *reinterpret_cast<T_type*>(&m_storage->operator[](m_index * m_storage->m_typesize)); }
		const T_type& operator* () const { return *reinterpret_cast<T_type*>(&m_storage->operator[](m_index * m_storage->m_typesize)); }

		void try_find_next_valid_index()
		{
			while (m_index < static_cast<int>(m_storage->size() / m_storage->m_typesize) &&
				!reinterpret_cast<T_type*>(&(*m_storage)[m_index * m_storage->m_typesize])->is_valid())
			{
				++m_index;
			}
		}

		Bucket_byte_allocator* m_storage = nullptr;
		i32 m_index = -1;
	};

	template <typename T_type>
	struct Bucket_allocator_view
	{
		using Iterator = Bucket_allocator_iterator<T_type>;
		using Const_iterator = Bucket_allocator_iterator<const T_type>;

		Bucket_allocator_view() = default;
		Bucket_allocator_view(Bucket_byte_allocator* in_storage) : m_storage(in_storage) {}

		Iterator begin() { return Iterator(m_storage, 0); }
		Iterator end() { return Iterator(m_storage, static_cast<i32>(m_storage->size() / m_storage->m_typesize)); }
		const Const_iterator begin() const { return Const_iterator(m_storage, 0); }
		const Const_iterator end() const { return Const_iterator(m_storage, static_cast<i32>(m_storage->size() / m_storage->m_typesize)); }

		Bucket_byte_allocator* m_storage = nullptr;
	};
}
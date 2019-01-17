#pragma once
#include "bucket_allocator.h"

template<typename component_type>
struct component_storage_handle
{
	size_t size() const
	{
		assert(m_array && "");

		return m_array->size() / sizeof(component_type);
	}

	component_type& operator[] (size_t in_index)
	{
		assert(m_array && "");

		return *reinterpret_cast<component_type*>(&m_array->operator[](in_index * sizeof(component_type)));
	}

	bucket_byte_allocator* m_array = nullptr;
};
#pragma once
#include "bucket_allocator.h"
#include "entity.h"

struct i_component
{
	bool is_valid() const
	{
		return m_owner != nullptr;
	}
	entity* m_owner = nullptr;
	int m_instance_index = -1;
};

struct component_storage
{
	template <typename component_type>
	void initialize(size_t in_initial_capacity, size_t in_bucket_capacity);

	template <typename component_type>
	component_type& create_component();

	template <typename component_type>
	void destroy_component(component_type& in_component_to_destroy);

	template <typename component_type>
	static size_t component_type_index()
	{
		static size_t index = index_ticker++;
		return index;
	}
	static size_t index_ticker;

	bucket_byte_allocator m_components;
	std::vector<size_t> m_free_indices;
	size_t m_component_type_size = 0;
	uint64_t m_component_flag_value = 0;
	size_t m_bucket_size = 0;
};

size_t component_storage::index_ticker = 0;

template<typename component_type>
inline void component_storage::initialize(size_t in_initial_capacity, size_t in_bucket_capacity)
{
	static_assert(std::is_base_of<i_component, component_type>::value, "component_type must derive from struct i_component");

	const size_t type_index = component_storage::component_type_index<component_type>();

	m_component_type_size = sizeof(component_type);
	m_bucket_size = in_bucket_capacity;

	m_components.allocate(in_initial_capacity * m_component_type_size, in_bucket_capacity * m_component_type_size);
}

template<typename component_type>
inline component_type& component_storage::create_component()
{
	static_assert(std::is_base_of<i_component, component_type>::value, "component_type must derive from struct i_component");

	if (!m_free_indices.empty())
	{
		size_t index = m_free_indices.back();

		byte& new_loc = m_components[index * sizeof(component_type)];
		new (&new_loc) component_type();

		m_free_indices.pop_back();

		component_type* new_instance = reinterpret_cast<component_type*>(&new_loc);
		new_instance->m_instance_index = index;
		return *new_instance;
	}

	const size_t index = m_components.size() / sizeof(component_type);

	component_type* new_instance = reinterpret_cast<component_type*>(&m_components.emplace_back(sizeof(component_type)));
	new_instance->m_instance_index = index;

	return *new_instance;
}

template<typename component_type>
inline void component_storage::destroy_component(component_type& in_component_to_destroy)
{
	static_assert(std::is_base_of<i_component, component_type>::value, "component_type must derive from struct i_component");

	const size_t index = in_component_to_destroy.m_instance_index;
	entity* owner = in_component_to_destroy.m_owner;

	m_free_indices.push_back(index);

	const size_t type_index = component_storage::component_type_index<component_type>();

	for (size_t i = 0; i < owner->m_components.size(); i++)
	{
		component_handle& handle = owner->m_components[i];

		if (handle.m_type_index == type_index &&
			handle.m_component->m_instance_index == index)
		{
			owner->m_components.erase(owner->m_components.begin() + i);
			break;
		}
	}

	in_component_to_destroy.m_instance_index = -1;
	in_component_to_destroy.m_owner = nullptr;
}

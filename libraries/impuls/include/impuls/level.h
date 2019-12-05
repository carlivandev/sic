#pragma once
#include "impuls/engine.h"
#include "impuls/component.h"
#include "impuls/object_base.h"
#include "impuls/bucket_allocator_view.h"

#include <vector>
#include <memory>

namespace impuls
{
	struct level
	{
		level(engine& in_engine) : m_engine(in_engine) {}

		template <typename t_component_type>
		typename t_component_type& create_component(i_object_base& in_object_to_attach_to);

		template <typename t_component_type>
		void destroy_component(t_component_type& in_component_to_destroy);

		template <typename t_type>
		plf::colony<t_type>& components();

		template <typename t_type>
		const plf::colony<t_type>& components() const;

		template <typename t_type>
		bucket_allocator_view<t_type> objects();

		template <typename t_type>
		bucket_allocator_view<const t_type> objects() const;

		std::unique_ptr<i_component_storage>& get_component_storage_at_index(i32 in_index);
		std::unique_ptr<i_object_storage_base>& get_object_storage_at_index(i32 in_index);

		std::vector<std::unique_ptr<i_component_storage>> m_component_storages;
		std::vector<std::unique_ptr<i_object_storage_base>> m_objects;

		engine& m_engine;
	};

	template<typename t_component_type>
	inline typename t_component_type& level::create_component(i_object_base& in_object_to_attach_to)
	{
		const ui32 type_idx = type_index<i_component_base>::get<t_component_type>();

		assert(type_idx < m_component_storages.size() && m_component_storages[type_idx]->initialized() && "component was not registered before use");

		component_storage<t_component_type>* storage = reinterpret_cast<component_storage<t_component_type>*>(m_component_storages[type_idx].get());
		auto& comp = storage->create_component();

		comp.m_owner = &in_object_to_attach_to;

		m_engine.invoke<event_created<t_component_type>>(comp);

		return comp;
	}

	template<typename t_component_type>
	inline void level::destroy_component(t_component_type& in_component_to_destroy)
	{
		const ui32 type_idx = type_index<i_component_base>::get<t_component_type>();
		m_engine.invoke<event_destroyed<t_component_type>>(in_component_to_destroy);

		component_storage<t_component_type>* storage = reinterpret_cast<component_storage<t_component_type>*>(m_component_storages[type_idx].get());
		storage->destroy_component(in_component_to_destroy);
	}

	template <typename t_type>
	__forceinline plf::colony<t_type>& level::components()
	{
		const ui32 type_idx = type_index<i_component_base>::get<t_type>();
		component_storage<t_type>* storage = reinterpret_cast<component_storage<t_type>*>(m_component_storages[type_idx].get());

		return storage->m_components;
	}

	template <typename t_type>
	__forceinline const plf::colony<t_type>& level::components() const
	{
		const ui32 type_idx = type_index<i_component_base>::get<t_type>();
		const component_storage<t_type>* storage = reinterpret_cast<const component_storage<t_type>*>(m_component_storages[type_idx].get());

		return storage->m_components;
	}

	template <typename t_type>
	__forceinline bucket_allocator_view<t_type> level::objects()
	{
		const ui32 type_idx = type_index<i_object_base>::get<t_type>();
		return std::move(bucket_allocator_view<t_type>(&m_objects[type_idx]->m_instances.m_byte_allocator));
	}

	template <typename t_type>
	__forceinline bucket_allocator_view<const t_type> level::objects() const
	{
		const ui32 type_idx = type_index<i_object_base>::get<t_type>();
		return std::move(bucket_allocator_view<const t_type>(&m_objects[type_idx]->m_instances.m_byte_allocator));
	}
}
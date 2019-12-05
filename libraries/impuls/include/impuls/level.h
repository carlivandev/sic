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
		void for_each(std::function<void(t_type&)> in_func);

		template <typename t_type>
		void for_each(std::function<void(const t_type&)> in_func) const;

		std::unique_ptr<i_component_storage>& get_component_storage_at_index(i32 in_index);
		std::unique_ptr<i_object_storage_base>& get_object_storage_at_index(i32 in_index);

		std::vector<std::unique_ptr<i_component_storage>> m_component_storages;
		std::vector<std::unique_ptr<i_object_storage_base>> m_objects;

		std::vector<std::unique_ptr<level>> m_sublevels;
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

	template<typename t_type>
	inline void level::for_each(std::function<void(t_type&)> in_func)
	{
		constexpr bool is_component = std::is_base_of<i_component_base, t_type>::value;
		constexpr bool is_object = std::is_base_of<i_object_base, t_type>::value;

		if constexpr (is_component)
		{
			const ui32 type_idx = type_index<i_component_base>::get<t_type>();
			component_storage<t_type>* storage = reinterpret_cast<component_storage<t_type>*>(m_component_storages[type_idx].get());

			for (t_type& component : storage->m_components)
				in_func(component);
		}
		else if constexpr (is_object)
		{
			const ui32 type_idx = type_index<i_object_base>::get<t_type>();
			bucket_allocator_view<t_type> object_view(&m_objects[type_idx]->m_instances.m_byte_allocator);

			for (t_type& object : object_view)
				in_func(object);
		}
		else
		{
			static_assert(false, "t_type not valid for for_each<t>. only objects and components supported.");
		}

		for (auto& sublevel : m_sublevels)
			sublevel->for_each<t_type>(in_func);
	}

	template<typename t_type>
	inline void level::for_each(std::function<void(const t_type&)> in_func) const
	{
		constexpr bool is_component = std::is_base_of<i_component_base, t_type>::value;
		constexpr bool is_object = std::is_base_of<i_object_base, t_type>::value;

		if constexpr (is_component)
		{
			const ui32 type_idx = type_index<i_component_base>::get<t_type>();
			const component_storage<t_type>* storage = reinterpret_cast<const component_storage<t_type>*>(m_component_storages[type_idx].get());

			for (const t_type& component : storage->m_components)
				in_func(component);
		}
		else if constexpr (is_object)
		{
			const ui32 type_idx = type_index<i_object_base>::get<t_type>();
			bucket_allocator_view<const t_type> object_view(&m_objects[type_idx]->m_instances.m_byte_allocator);

			for (const t_type& object : object_view)
				in_func(object);
		}
		else
		{
			static_assert(false, "t_type not valid for for_each<t>. only objects and components supported.");
		}

		for (auto& sublevel : m_sublevels)
			sublevel->for_each<t_type>(in_func);
	}
}
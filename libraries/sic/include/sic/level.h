#pragma once
#include "sic/engine.h"
#include "sic/component.h"
#include "sic/object_base.h"
#include "sic/bucket_allocator_view.h"

#include <vector>
#include <memory>

namespace sic
{
	struct Level : Noncopyable
	{
		Level(Engine& in_engine) : m_engine(in_engine) {}

		template <typename t_object>
		constexpr t_object& create_object();

		void destroy_object(Object_base& in_object_to_destroy);

		template <typename t_component_type>
		typename t_component_type& create_component(Object_base& in_object_to_attach_to);

		template <typename t_component_type>
		void destroy_component(t_component_type& in_component_to_destroy);

		template <typename t_type>
		void for_each(std::function<void(t_type&)> in_func);

		template <typename t_type>
		void for_each(std::function<void(const t_type&)> in_func) const;

		std::unique_ptr<Component_storage_base>& get_component_storage_at_index(i32 in_index);
		std::unique_ptr<Object_storage_base>& get_object_storage_at_index(i32 in_index);

		std::vector<std::unique_ptr<Component_storage_base>> m_component_storages;
		std::vector<std::unique_ptr<Object_storage_base>> m_objects;

		std::vector<std::unique_ptr<Level>> m_sublevels;
		Engine& m_engine;

		i32 m_level_id = -1;
	};

	template <typename t_object>
	inline constexpr t_object& Level::create_object()
	{
		static_assert(std::is_base_of<Object_base, t_object>::value, "object must derive from struct Object<>");

		const ui32 type_idx = Type_index<Object_base>::get<t_object>();

		assert((type_idx < m_objects.size() || m_objects[type_idx].get() != nullptr) && "type not registered");

		auto& arch_to_create_from = m_objects[type_idx];

		Level_context context(m_engine, *this);
		t_object& new_instance = reinterpret_cast<Object_storage*>(arch_to_create_from.get())->make_instance<t_object>(context);

		m_engine.invoke<event_created<t_object>>(new_instance);

		return new_instance;
	}

	template<typename t_component_type>
	inline typename t_component_type& Level::create_component(Object_base& in_object_to_attach_to)
	{
		const ui32 type_idx = Type_index<Component_base>::get<t_component_type>();

		assert(type_idx < m_component_storages.size() && m_component_storages[type_idx]->initialized() && "component was not registered before use");

		Component_storage<t_component_type>* storage = reinterpret_cast<Component_storage<t_component_type>*>(m_component_storages[type_idx].get());
		auto& comp = storage->create_component();

		comp.m_owner = &in_object_to_attach_to;

		m_engine.invoke<event_created<t_component_type>>(comp);

		return comp;
	}

	template<typename t_component_type>
	inline void Level::destroy_component(t_component_type& in_component_to_destroy)
	{
		const ui32 type_idx = Type_index<Component_base>::get<t_component_type>();
		m_engine.invoke<event_destroyed<t_component_type>>(in_component_to_destroy);

		Component_storage<t_component_type>* storage = reinterpret_cast<Component_storage<t_component_type>*>(m_component_storages[type_idx].get());
		storage->destroy_component(in_component_to_destroy);
	}

	template<typename t_type>
	inline void Level::for_each(std::function<void(t_type&)> in_func)
	{
		constexpr bool is_component = std::is_base_of<Component_base, t_type>::value;
		constexpr bool is_object = std::is_base_of<Object_base, t_type>::value;

		if constexpr (is_component)
		{
			const ui32 type_idx = Type_index<Component_base>::get<t_type>();
			Component_storage<t_type>* storage = reinterpret_cast<Component_storage<t_type>*>(m_component_storages[type_idx].get());

			for (t_type& component : storage->m_components)
				in_func(component);
		}
		else if constexpr (is_object)
		{
			const ui32 type_idx = Type_index<Object_base>::get<t_type>();
			Bucket_allocator_view<t_type> object_view(&m_objects[type_idx]->m_instances.m_byte_allocator);

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
	inline void Level::for_each(std::function<void(const t_type&)> in_func) const
	{
		constexpr bool is_component = std::is_base_of<Component_base, t_type>::value;
		constexpr bool is_object = std::is_base_of<Object_base, t_type>::value;

		if constexpr (is_component)
		{
			const ui32 type_idx = Type_index<Component_base>::get<t_type>();
			const Component_storage<t_type>* storage = reinterpret_cast<const Component_storage<t_type>*>(m_component_storages[type_idx].get());

			for (const t_type& component : storage->m_components)
				in_func(component);
		}
		else if constexpr (is_object)
		{
			const ui32 type_idx = Type_index<Object_base>::get<t_type>();
			Bucket_allocator_view<const t_type> object_view(&m_objects[type_idx]->m_instances.m_byte_allocator);

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
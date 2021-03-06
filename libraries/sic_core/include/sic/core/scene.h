#pragma once
#include "sic/core/engine.h"
#include "sic/core/component.h"
#include "sic/core/object_base.h"
#include "sic/core/bucket_allocator_view.h"

#include <vector>
#include <memory>
#include <mutex>

namespace sic
{
	struct Scene : Noncopyable
	{
		Scene(Engine& in_engine, Scene* in_outermost_scene, Scene* in_parent_scene) : m_engine(in_engine), m_outermost_scene(in_outermost_scene) , m_parent_scene(in_parent_scene)
		{
			m_object_exists_flags.reserve(512);
		}

		template <typename T_object>
		constexpr T_object& create_object();

		void destroy_object(Object_base& in_object_to_destroy);
		//carl: not threadsafe!
		void destroy_object_immediate(Object_base& inout_object_to_destroy);

		template <typename T_component_type>
		typename T_component_type& create_component(Object_base& in_object_to_attach_to);

		template <typename T_component_type>
		void destroy_component(T_component_type& in_component_to_destroy);

		template <typename T_type>
		void for_each_w(std::function<void(T_type&)> in_func);

		template <typename T_type>
		void for_each_r(std::function<void(const T_type&)> in_func) const;

		bool get_is_root_scene() const { return m_outermost_scene == nullptr; }

		bool get_does_object_exist(Object_id in_id, bool in_only_in_this_scene) const;

		std::unique_ptr<Component_storage_base>& get_component_storage_at_index(i32 in_index);
		std::unique_ptr<Object_storage_base>& get_object_storage_at_index(i32 in_index);

		std::vector<std::unique_ptr<Component_storage_base>> m_component_storages;
		std::vector<std::unique_ptr<Object_storage_base>> m_objects;

		std::vector<std::unique_ptr<Scene>> m_subscenes;
		Engine& m_engine;

		std::vector<bool> m_object_exists_flags;
		Scene* m_outermost_scene = nullptr;
		Scene* m_parent_scene = nullptr;
		i32 m_scene_id = -1;
		i32 m_current_object_id_ticker = 0;

		std::function<void()> m_on_created_callback;

		mutable std::mutex m_object_creation_mutex;
	};

	template <typename T_object>
	inline constexpr T_object& Scene::create_object()
	{
		static_assert(std::is_base_of<Object_base, T_object>::value, "object must derive from struct Object<>");

		const ui32 type_idx = Type_index<Object_base>::get<T_object>();

		assert((type_idx < m_objects.size() || m_objects[type_idx].get() != nullptr) && "type not registered");

		auto& arch_to_create_from = m_objects[type_idx];

		Scene_context context(m_engine, *this);
		T_object& new_instance = reinterpret_cast<Object_storage*>(arch_to_create_from.get())->make_instance<T_object>(context);
		new_instance.m_id = m_current_object_id_ticker++;

		{
			std::scoped_lock lock(m_object_creation_mutex);
			m_object_exists_flags.resize(new_instance.m_id + 1);
			m_object_exists_flags[new_instance.m_id] = true;
		}

		m_engine.invoke<Event_created<T_object>>(std::reference_wrapper<T_object>(new_instance));

		new_instance.invoke_post_creation_events(context);

		return new_instance;
	}

	template<typename T_component_type>
	inline typename T_component_type& Scene::create_component(Object_base& in_object_to_attach_to)
	{
		const ui32 type_idx = Type_index<Component_base>::get<T_component_type>();

		assert(type_idx < m_component_storages.size() && m_component_storages[type_idx]->initialized() && "component was not registered before use");

		Component_storage<T_component_type>* storage = reinterpret_cast<Component_storage<T_component_type>*>(m_component_storages[type_idx].get());
		auto& comp = storage->create_component();

		comp.m_owner = &in_object_to_attach_to;

		m_engine.invoke<Event_created<T_component_type>>(std::reference_wrapper<T_component_type>(comp));

		return comp;
	}

	template<typename T_component_type>
	inline void Scene::destroy_component(T_component_type& in_component_to_destroy)
	{
		const ui32 type_idx = Type_index<Component_base>::get<T_component_type>();
		m_engine.invoke<Event_destroyed<T_component_type>>(std::reference_wrapper<T_component_type>(in_component_to_destroy));

		Component_storage<T_component_type>* storage = reinterpret_cast<Component_storage<T_component_type>*>(m_component_storages[type_idx].get());
		storage->destroy_component(in_component_to_destroy);
	}

	template<typename T_type>
	inline void Scene::for_each_w(std::function<void(T_type&)> in_func)
	{
		constexpr bool is_component = std::is_base_of<Component_base, T_type>::value;
		constexpr bool is_object = std::is_base_of<Object_base, T_type>::value;

		if constexpr (is_component)
		{
			const ui32 type_idx = Type_index<Component_base>::get<T_type>();
			Component_storage<T_type>* storage = reinterpret_cast<Component_storage<T_type>*>(m_component_storages[type_idx].get());

			for (T_type& component : storage->m_components)
				in_func(component);
		}
		else if constexpr (is_object)
		{
			const ui32 type_idx = Type_index<Object_base>::get<T_type>();
			Bucket_allocator_view<T_type> object_view(&m_objects[type_idx]->m_instances.m_byte_allocator);

			for (T_type& object : object_view)
				in_func(object);
		}
		else
		{
			static_assert(false, "T_type not valid for for_each<t>. only objects and components supported.");
		}

		for (auto& subscene : m_subscenes)
			subscene->for_each_w<T_type>(in_func);
	}

	template<typename T_type>
	inline void Scene::for_each_r(std::function<void(const T_type&)> in_func) const
	{
		constexpr bool is_component = std::is_base_of<Component_base, T_type>::value;
		constexpr bool is_object = std::is_base_of<Object_base, T_type>::value;

		if constexpr (is_component)
		{
			const ui32 type_idx = Type_index<Component_base>::get<T_type>();
			const Component_storage<T_type>* storage = reinterpret_cast<const Component_storage<T_type>*>(m_component_storages[type_idx].get());

			for (const T_type& component : storage->m_components)
				in_func(component);
		}
		else if constexpr (is_object)
		{
			const ui32 type_idx = Type_index<Object_base>::get<T_type>();
			Bucket_allocator_view<const T_type> object_view(&m_objects[type_idx]->m_instances.m_byte_allocator);

			for (const T_type& object : object_view)
				in_func(object);
		}
		else
		{
			static_assert(false, "T_type not valid for for_each<t>. only objects and components supported.");
		}

		for (auto& subscene : m_subscenes)
			subscene->for_each_w<T_type>(in_func);
	}
}
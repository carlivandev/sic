#pragma once
#include "component.h"
#include "component_view.h"
#include "event.h"
#include "scheduler.h"
#include "system_base.h"
#include "threadpool.h"

#include <new>
#include <chrono>
#include <unordered_map>

namespace impuls
{
	struct world_event_index
	{
		template <typename event_type>
		static ui32 get()
		{
			static ui32 index = m_event_index_ticker++;
			return index;
		}
		static ui32 m_event_index_ticker;
	};

	struct world
	{
		void initialize(ui32 in_initial_object_capacity = 128, ui32 in_object_bucket_capacity = 64);

		void simulate();
		void begin_simulation();
		void tick();
		void end_simulation();

		void destroy() { m_is_destroyed = true; }
		bool is_destroyed() const { return m_is_destroyed; }

		template <typename component_type>
		void register_component_type(ui32 in_initial_capacity = 128, ui32 in_bucket_capacity = 64);

		template <typename system_type>
		system_type& create_system();

		template <typename system_type>
		system_type& create_system(const std::vector<i_system_base*>& in_dependencies);

		template <typename system_type>
		system_type& create_system_internal();

		object& create_object();

		void destroy_object(object& in_object_to_destroy);

		template <typename component_type>
		component_type& create_component(object& in_object_to_attach_to);

		template <typename component_type>
		void destroy_component(component_type& in_component_to_destroy);

		template <typename component_type>
		component_type* get_component(object& in_object_to_get_from);

		template <typename component_type>
		component_view<component_type, component_storage> all();

		template <typename component_type>
		const component_view<component_type, const component_storage> all() const;

		template <typename event_type, typename functor>
		void listen(functor in_func);

		template <typename event_type, typename event_data>
		void invoke(event_data& event_data_to_send);

		void refresh_time_delta();

		std::vector<std::unique_ptr<component_storage>> m_component_storages;
		std::vector<std::unique_ptr<i_system_base>> m_systems;
		bucket_allocator<object> m_objects;
		std::vector<i32> m_object_free_indices;

		std::vector<std::unique_ptr<event_base>> m_world_events;
		std::unordered_map<ui32, ui32> m_component_type_index_to_destroyed_event_index_lut;
		std::chrono::time_point<std::chrono::high_resolution_clock> m_previous_frame_time_point;
		float m_time_delta = -1.0f;

		scheduler m_scheduler;
		threadpool m_threadpool;

		bool m_is_destroyed = true;
		bool m_initialized = false;
		bool m_begun_simulation = false;
	};

	template<typename component_type>
	inline void world::register_component_type(ui32 in_initial_capacity, ui32 in_bucket_capacity)
	{
		const ui32 type_index = component_storage::component_type_index<component_type>();

		while (type_index >= m_component_storages.size())
			m_component_storages.push_back(std::make_unique<component_storage>());

		m_component_storages[type_index]->initialize<component_type>(in_initial_capacity, in_bucket_capacity);

		m_component_type_index_to_destroyed_event_index_lut[type_index] = world_event_index::get<on_component_destroyed<component_type>>();
	}

	template<typename system_type>
	inline system_type& world::create_system()
	{
		system_type& sys = create_system_internal<system_type>();

		m_scheduler.schedule(&sys, {});

		return sys;
	}

	template<typename system_type>
	inline system_type& world::create_system(const std::vector<i_system_base*>& in_dependencies)
	{
		system_type& sys = create_system_internal<system_type>();

		m_scheduler.schedule(&sys, in_dependencies);

		return sys;
	}

	template<typename system_type>
	inline system_type& world::create_system_internal()
	{
		static_assert(std::is_base_of<i_system_base, system_type>::value, "system_type must derive from struct i_system_base");

		m_systems.push_back(std::move(std::make_unique<system_type>()));
		m_systems.back()->m_name = typeid(system_type).name();
		m_systems.back()->m_world = this;
		m_systems.back()->on_created();
		
		return *reinterpret_cast<system_type*>(m_systems.back().get());
	}

	template<typename component_type>
	inline component_type& world::create_component(object& in_object_to_attach_to)
	{
		const ui32 type_index = component_storage::component_type_index<component_type>();

		component_type& new_instance = m_component_storages[type_index]->create_component<component_type>();
		new_instance.m_owner = &in_object_to_attach_to;

		in_object_to_attach_to.m_components.push_back(component_handle(type_index, &new_instance));

		invoke<on_component_created<component_type>>(new_instance);

		return new_instance;
	}

	template<typename component_type>
	inline void world::destroy_component(component_type& in_component_to_destroy)
	{
		const ui32 type_index = component_storage::component_type_index<component_type>();
		invoke<on_component_destroyed<component_type>>(in_component_to_destroy);

		m_component_storages[type_index]->destroy_component<component_type>(in_component_to_destroy);
	}

	template<typename component_type>
	inline component_type* world::get_component(object& in_object_to_get_from)
	{
		const ui32 type_index = component_storage::component_type_index<component_type>();

		for (component_handle& handle : in_object_to_get_from.m_components)
		{
			if (handle.m_type_index == type_index)
				return reinterpret_cast<component_type*>(&m_component_storages[type_index]->m_components[handle.m_component->m_instance_index * sizeof(component_type)]);
		}

		return nullptr;
	}

	template<typename component_type>
	inline component_view<component_type, component_storage> world::all()
	{
		const ui32 type_index = component_storage::component_type_index<component_type>();
		return component_view<component_type, component_storage>(m_component_storages[type_index].get());
	}

	template<typename component_type>
	inline const component_view<component_type, const component_storage> world::all() const
	{
		const ui32 type_index = component_storage::component_type_index<component_type>();
		return component_view<component_type, const component_storage>(m_component_storages[type_index].get());
	}

	template<typename event_type, typename functor>
	inline void world::listen(functor in_func)
	{
		const ui32 event_index = world_event_index::get<event_type>();

		while (event_index >= m_world_events.size())
			m_world_events.emplace_back();

		auto& world_event_base = m_world_events[event_index];

		if (!world_event_base)
			world_event_base = std::make_unique<event_type>();

		event_type* world_event = reinterpret_cast<event_type*>(world_event_base.get());

		world_event->m_listeners.push_back(in_func);
	}

	template<typename event_type, typename event_data>
	inline void world::invoke(event_data& event_data_to_send)
	{
		const ui32 event_index = world_event_index::get<event_type>();

		if (event_index >= m_world_events.size())
			return;

		auto& world_event_base = m_world_events[event_index];

		if (!world_event_base)
			return;

		event_type* world_event = reinterpret_cast<event_type*>(world_event_base.get());

		world_event->invoke(event_data_to_send);
	}
}
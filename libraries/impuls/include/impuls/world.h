#pragma once
#include "component.h"

#include "object_base.h"
#include "system.h"

#include "event.h"

#include "scheduler.h"
#include "threadpool.h"

#include "type_index.h"
#include "type.h"

#include <new>
#include <chrono>
#include <unordered_map>

namespace impuls
{
	//worldwide state data
	struct i_state
	{
	};

	struct world
	{
		template <typename ...t_systems>
		friend struct tickstep_async;

		template <typename ...t_systems>
		friend struct tickstep_synced;

		void initialize();

		void simulate();
		void begin_simulation();
		void tick();
		void end_simulation();

		void destroy() { m_is_destroyed = true; }
		bool is_destroyed() const { return m_is_destroyed; }

		template <typename ...t_steps>
		void set_ticksteps();

		template <typename t_component_type>
		void register_component_type(ui32 in_initial_capacity = 128);

		template <typename t_system_type>
		t_system_type& create_system();

		template <typename t_system_type>
		t_system_type& create_system_internal();

		template <typename t_component_type>
		typename plf::colony<t_component_type>::iterator create_component(i_object_base& in_object_to_attach_to);

		template <typename t_component_type>
		void destroy_component(typename plf::colony<t_component_type>::iterator in_component_to_destroy);

		template <typename t_event_type, typename t_functor>
		void listen(t_functor in_func);

		template <typename t_event_type, typename event_data>
		void invoke(event_data& event_data_to_send);

		void refresh_time_delta();

		std::unique_ptr<i_component_storage>& get_component_storage_at_index(i32 in_index);
		std::unique_ptr<i_object_storage_base>& get_object_storage_at_index(i32 in_index);
		std::unique_ptr<i_state>& get_state_at_index(i32 in_index);

		std::vector<std::unique_ptr<i_system>> m_systems;
		std::vector<std::unique_ptr<i_component_storage>> m_component_storages;
		std::vector<std::unique_ptr<i_object_storage_base>> m_objects;
		std::vector<std::unique_ptr<i_state>> m_states;

		std::vector<std::unique_ptr<i_event_base>> m_world_events;

		std::vector<typeinfo*> m_typeinfos;
		std::vector<typeinfo*> m_component_typeinfos;
		std::vector<typeinfo*> m_object_typeinfos;
		std::vector<typeinfo*> m_state_typeinfos;
		std::unordered_map<std::string, std::unique_ptr<typeinfo>> m_typename_to_typeinfo_lut;

		std::vector<i_system*> m_async_ticksteps;
		std::vector<std::vector<i_system*>> m_synced_ticksteps;
		threadpool m_tickstep_threadpool;

		std::chrono::time_point<std::chrono::high_resolution_clock> m_previous_frame_time_point;
		float m_time_delta = -1.0f;

		bool m_is_destroyed = true;
		bool m_initialized = false;
		bool m_begun_simulation = false;
	};

	template <typename ...t_systems>
	struct tickstep_async
	{
		template <typename t_system>
		static void add_to_tickstep(world& in_world)
		{
			in_world.m_async_ticksteps.push_back(&in_world.create_system<t_system>());
		}

		static void add_tickstep(world& in_world)
		{
			(add_to_tickstep<t_systems>(in_world), ...);
		}
	};

	//first system will always execute on main thread
	template <typename ...t_systems>
	struct tickstep_synced
	{
		template <typename t_system>
		static void add_to_tickstep(world& in_world, std::vector<i_system*>& out_tickstep)
		{
			out_tickstep.push_back(&in_world.create_system<t_system>());
		}

		static void add_tickstep(world& in_world)
		{
			auto& tickstep = in_world.m_synced_ticksteps.emplace_back();
			(add_to_tickstep<t_systems>(in_world, tickstep), ...);
		}
	};

	template<typename ...t_steps>
	inline void world::set_ticksteps()
	{
		(t_steps::add_tickstep(*this), ...);
	}

	template<typename t_component_type>
	inline void world::register_component_type(ui32 in_initial_capacity)
	{
		const ui32 type_idx = type_index<i_component_base>::get<t_component_type>();

		while (type_idx >= m_component_storages.size())
			m_component_storages.push_back(nullptr);

		if (!m_component_storages[type_idx].get())
		{
			component_storage<t_component_type>* new_storage = new component_storage<t_component_type>();
			new_storage->initialize(in_initial_capacity);
			m_component_storages[type_idx] = std::unique_ptr<i_component_storage>(new_storage);
		}
	}

	template<typename t_system_type>
	inline t_system_type& world::create_system()
	{
		t_system_type& sys = create_system_internal<t_system_type>();

		return sys;
	}

	template<typename t_system_type>
	inline t_system_type& world::create_system_internal()
	{
		static_assert(std::is_base_of<i_system, t_system_type>::value, "system_type must derive from struct i_system");

		const ui64 new_system_idx = m_systems.size();

		m_systems.push_back(std::move(std::make_unique<t_system_type>()));
		
		m_systems.back()->m_name = typeid(t_system_type).name();
		m_systems.back()->on_created(std::move(world_context(*this, *m_systems.back().get())));
		
		return *reinterpret_cast<t_system_type*>(m_systems[new_system_idx].get());
	}

	template<typename t_component_type>
	inline typename plf::colony<t_component_type>::iterator world::create_component(i_object_base& in_object_to_attach_to)
	{
		const ui32 type_idx = type_index<i_component_base>::get<t_component_type>();

		assert(type_idx < m_component_storages.size() && m_component_storages[type_idx]->initialized() && "component was not registered before use");

		auto storage = reinterpret_cast<component_storage<t_component_type>*>(m_component_storages[type_idx].get());
		auto it = storage->create_component();

		it->m_owner = &in_object_to_attach_to;

		invoke<event_created<t_component_type>>(*it);

		return it;
	}

	template<typename t_component_type>
	inline void world::destroy_component(typename plf::colony<t_component_type>::iterator in_component_to_destroy)
	{
		const ui32 type_idx = type_index<i_component_base>::get<t_component_type>();
		invoke<event_destroyed<t_component_type>>(*in_component_to_destroy);

		auto storage = reinterpret_cast<component_storage<t_component_type>*>(m_component_storages[type_idx].get());
		storage->destroy_component(in_component_to_destroy);
	}

	template<typename t_event_type, typename t_functor>
	inline void world::listen(t_functor in_func)
	{
		const ui32 type_idx = type_index<i_event_base>::get<t_event_type>();

		while (type_idx >= m_world_events.size())
			m_world_events.emplace_back();

		auto& world_event_base = m_world_events[type_idx];

		if (!world_event_base)
			world_event_base = std::make_unique<t_event_type>();

		t_event_type* world_event = reinterpret_cast<t_event_type*>(world_event_base.get());

		world_event->m_listeners.push_back(in_func);
	}

	template<typename t_event_type, typename event_data>
	inline void world::invoke(event_data& event_data_to_send)
	{
		const ui32 type_idx = type_index<i_event_base>::get<t_event_type>();

		if (type_idx >= m_world_events.size())
			return;

		auto& world_event_base = m_world_events[type_idx];

		if (!world_event_base)
			return;

		t_event_type* world_event = reinterpret_cast<t_event_type*>(world_event_base.get());

		world_event->invoke(world_context(*this), event_data_to_send);
	}
}
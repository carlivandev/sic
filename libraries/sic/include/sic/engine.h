#pragma once
#include "sic/type_restrictions.h"
#include "sic/type_index.h"
#include "sic/type.h"

#include "sic/component.h"
#include "sic/object_base.h"

#include "sic/system.h"
#include "sic/event.h"

#include "sic/threadpool.h"

#include <new>
#include <chrono>
#include <unordered_map>

namespace sic
{
	enum struct Tickstep
	{
		async, //never synchronizes
		pre_tick, //runs before tick, always on main thread
		tick, //everything here runs in parallel, first added runs on main thread
		post_tick //runs after tick, always on main thread
	};

	struct Level;

	//enginewide state data
	struct State : Noncopyable
	{
	};

	struct Engine : Noncopyable
	{
		friend struct Engine_context;

		void initialize();

		void simulate();
		void prepare_threadpool();
		void tick();
		void on_shutdown();

		void shutdown() { m_is_shutting_down = true; }
		bool is_shutting_down() const { return m_is_shutting_down; }

		template <typename t_component_type>
		constexpr Type_reg register_component_type(const char* in_unique_key, ui32 in_initial_capacity = 128);

		template <typename t_object>
		constexpr Type_reg register_object(const char* in_unique_key, ui32 in_initial_capacity = 128, ui32 in_bucket_capacity = 64);

		template <typename t_type_to_register>
		constexpr Type_reg register_typeinfo(const char* in_unique_key);

		template <typename t_system>
		void add_system(Tickstep in_tickstep);

		template <typename t_system_type>
		t_system_type& create_system();

		template <typename t_system_type>
		t_system_type& create_system_internal();

		template <typename t_component_type>
		typename t_component_type& create_component(Object_base& in_object_to_attach_to);

		template <typename t_component_type>
		void destroy_component(t_component_type& in_component_to_destroy);

		template <typename t_state>
		t_state* get_state();

		template <typename t_type>
		constexpr Typeinfo* get_typeinfo();

		void create_level(Level* in_parent_level);
		void destroy_level(Level& inout_level);

		template <typename t_event_type, typename t_functor>
		void listen(t_functor in_func);

		template <typename t_event_type, typename event_data>
		void invoke(event_data& event_data_to_send);

		void refresh_time_delta();
		void flush_level_streaming();

	private:
		std::unique_ptr<State>& get_state_at_index(i32 in_index);
		void destroy_level_internal(Level& in_level);

		std::vector<std::unique_ptr<System>> m_systems;
		std::vector<std::unique_ptr<State>> m_states;
		
		std::vector<std::unique_ptr<Level>> m_levels;
		std::vector<std::unique_ptr<Level>> m_levels_to_add;
		std::vector<Level*> m_levels_to_remove;
		i32 m_level_id_ticker = 0;

		std::vector<std::unique_ptr<Event_base>> m_engine_events;

		std::vector<Typeinfo*> m_typeinfos;
		std::vector<Typeinfo*> m_component_typeinfos;
		std::vector<Typeinfo*> m_object_typeinfos;
		std::vector<Typeinfo*> m_state_typeinfos;
		std::unordered_map<std::string, std::unique_ptr<Typeinfo>> m_typename_to_typeinfo_lut;

		std::vector<System*> m_async_systems;
		std::vector<System*> m_pre_tick_systems;
		std::vector<System*> m_tick_systems;
		std::vector<System*> m_post_tick_systems;

		Threadpool m_system_ticker_threadpool;

		std::chrono::time_point<std::chrono::high_resolution_clock> m_previous_frame_time_point;
		float m_time_delta = -1.0f;

		bool m_is_shutting_down = true;
		bool m_initialized = false;
		bool m_has_prepared_threadpool = false;
		bool m_finished_setup = false;

		std::mutex m_levels_mutex;

		//callbacks to run whenever a new level is created
		std::vector<std::function<void(Level&)>> m_registration_callbacks;
	};

	template<typename t_component_type>
	inline constexpr Type_reg Engine::register_component_type(const char* in_unique_key, ui32 in_initial_capacity)
	{
		m_registration_callbacks.push_back
		(
			[in_initial_capacity](Level& inout_level)
			{
				const ui32 type_idx = Type_index<Component_base>::get<t_component_type>();

				while (type_idx >= inout_level.m_component_storages.size())
					inout_level.m_component_storages.push_back(nullptr);

				Component_storage<t_component_type>* new_storage = new Component_storage<t_component_type>();
				new_storage->initialize(in_initial_capacity);
				inout_level.m_component_storages[type_idx] = std::unique_ptr<Component_storage_base>(new_storage);
			}
		);

		return register_typeinfo<t_component_type>(in_unique_key);
	}

	template<typename t_object>
	inline constexpr Type_reg Engine::register_object(const char* in_unique_key, ui32 in_initial_capacity, ui32 in_bucket_capacity)
	{
		static_assert(std::is_base_of<Object_base, t_object>::value, "object must derive from struct Object<>");

		m_registration_callbacks.push_back
		(
			[in_initial_capacity, in_bucket_capacity](Level & inout_level)
			{
				const ui32 type_idx = Type_index<Object_base>::get<t_object>();

				while (type_idx >= inout_level.m_objects.size())
					inout_level.m_objects.push_back(nullptr);

				auto & new_object_storage = inout_level.get_object_storage_at_index(type_idx);

				assert(new_object_storage.get() == nullptr && "object is already registered");

				new_object_storage = std::make_unique<Object_storage>();
				reinterpret_cast<Object_storage*>(new_object_storage.get())->initialize_with_typesize(in_initial_capacity, in_bucket_capacity, sizeof(t_object));

			}
		);

		return register_typeinfo<t_object>(in_unique_key);
	}

	template<typename t_type_to_register>
	inline constexpr Type_reg Engine::register_typeinfo(const char* in_unique_key)
	{
		const char* type_name = typeid(t_type_to_register).name();

		assert(m_typename_to_typeinfo_lut.find(type_name) == m_typename_to_typeinfo_lut.end() && "typeinfo already registered!");

		auto & new_typeinfo = m_typename_to_typeinfo_lut[type_name] = std::make_unique<Typeinfo>();
		new_typeinfo->m_name = type_name;
		new_typeinfo->m_unique_key = in_unique_key;

		constexpr bool is_component = std::is_base_of<Component_base, t_type_to_register>::value;
		constexpr bool is_object = std::is_base_of<Object_base, t_type_to_register>::value;
		constexpr bool is_state = std::is_base_of<State, t_type_to_register>::value;

		if constexpr (is_component)
		{
			const ui32 type_idx = Type_index<Component_base>::get<t_type_to_register>();

			while (type_idx >= m_component_typeinfos.size())
				m_component_typeinfos.push_back(nullptr);

			m_component_typeinfos[type_idx] = new_typeinfo.get();
		}
		else if constexpr (is_object)
		{
			const ui32 type_idx = Type_index<Object_base>::get<t_type_to_register>();

			while (type_idx >= m_object_typeinfos.size())
				m_object_typeinfos.push_back(nullptr);

			m_object_typeinfos[type_idx] = new_typeinfo.get();
		}
		else if constexpr (is_state)
		{
			const ui32 type_idx = Type_index<State>::get<t_type_to_register>();

			while (type_idx >= m_state_typeinfos.size())
				m_state_typeinfos.push_back(nullptr);

			m_state_typeinfos[type_idx] = new_typeinfo.get();
		}
		else
		{
			const i32 type_idx = Type_index<Typeinfo>::get<t_type_to_register>();

			while (type_idx >= m_typeinfos.size())
				m_typeinfos.push_back(nullptr);

			m_typeinfos[type_idx] = new_typeinfo.get();
		}

		return Type_reg(new_typeinfo.get());
	}

	template<typename t_system>
	inline void Engine::add_system(Tickstep in_tickstep)
	{
		switch (in_tickstep)
		{
		case sic::Tickstep::async:
			m_async_systems.push_back(&create_system<t_system>());
			break;
		case sic::Tickstep::pre_tick:
			m_pre_tick_systems.push_back(&create_system<t_system>());
			break;
		case sic::Tickstep::tick:
			m_tick_systems.push_back(&create_system<t_system>());
			break;
		case sic::Tickstep::post_tick:
			m_post_tick_systems.push_back(&create_system<t_system>());
			break;
		default:
			break;
		}
	}

	template<typename t_system_type>
	inline t_system_type& Engine::create_system()
	{
		t_system_type& sys = create_system_internal<t_system_type>();

		return sys;
	}

	template<typename t_system_type>
	inline t_system_type& Engine::create_system_internal()
	{
		static_assert(std::is_base_of<System, t_system_type>::value, "system_type must derive from struct System");

		const ui64 new_system_idx = m_systems.size();

		m_systems.push_back(std::move(std::make_unique<t_system_type>()));
		
		m_systems.back()->m_name = typeid(t_system_type).name();
		m_systems.back()->on_created(std::move(Engine_context(*this)));
		
		return *reinterpret_cast<t_system_type*>(m_systems[new_system_idx].get());
	}

	template<typename t_component_type>
	inline typename t_component_type& Engine::create_component(Object_base& in_object_to_attach_to)
	{
		const ui32 type_idx = Type_index<Component_base>::get<t_component_type>();

		assert(type_idx < m_component_storages.size() && m_component_storages[type_idx]->initialized() && "component was not registered before use");

		Component_storage<t_component_type>* storage = reinterpret_cast<Component_storage<t_component_type>*>(m_component_storages[type_idx].get());
		auto& comp = storage->create_component();

		comp.m_owner = &in_object_to_attach_to;

		invoke<event_created<t_component_type>>(comp);

		return comp;
	}

	template<typename t_component_type>
	inline void Engine::destroy_component(t_component_type& in_component_to_destroy)
	{
		const ui32 type_idx = Type_index<Component_base>::get<t_component_type>();
		invoke<event_destroyed<t_component_type>>(in_component_to_destroy);

		Component_storage<t_component_type>* storage = reinterpret_cast<Component_storage<t_component_type>*>(m_component_storages[type_idx].get());
		storage->destroy_component(in_component_to_destroy);
	}

	template<typename t_state>
	inline t_state* Engine::get_state()
	{
		constexpr bool is_valid_type = std::is_base_of<State, t_state>::value;

		static_assert(is_valid_type, "can only get types that derive State");

		const i32 type_idx = Type_index<State>::get<t_state>();

		assert((type_idx < m_states.size() && m_states[type_idx].get() != nullptr) && "state not registered");

		return reinterpret_cast<t_state*>(m_states[type_idx].get());
	}

	template<typename t_type>
	inline constexpr Typeinfo* Engine::get_typeinfo()
	{
		constexpr bool is_component = std::is_base_of<Component_base, t_type>::value;
		constexpr bool is_object = std::is_base_of<Object_base, t_type>::value;
		constexpr bool is_state = std::is_base_of<State, t_type>::value;

		if constexpr (is_component)
		{
			const ui32 type_idx = Type_index<Component_base>::get<t_type>();
			assert(type_idx < m_engine.m_component_typeinfos.size() && m_engine.m_component_typeinfos[type_idx] != nullptr && "typeinfo not registered!");

			return m_engine.m_component_typeinfos[type_idx];
		}
		else if constexpr (is_object)
		{
			const ui32 type_idx = Type_index<Object_base>::get<t_type>();
			assert(type_idx < m_engine.m_object_typeinfos.size() && m_engine.m_object_typeinfos[type_idx] != nullptr && "typeinfo not registered!");

			return m_engine.m_object_typeinfos[type_idx];
		}
		else if constexpr (is_state)
		{
			const ui32 type_idx = Type_index<State>::get<t_type>();
			assert(type_idx < m_engine.m_states.size() && m_engine.m_states[type_idx] != nullptr && "typeinfo not registered!");

			return m_engine.m_states[type_idx];
		}
		else
		{
			const ui32 type_idx = Type_index<Typeinfo>::get<t_type>();
			assert(type_idx < m_engine.m_typeinfos.size() && m_engine.m_typeinfos[type_idx] != nullptr && "typeinfo not registered!");

			return m_engine.m_typeinfos[type_idx];
		}
	}

	template<typename t_event_type, typename t_functor>
	inline void Engine::listen(t_functor in_func)
	{
		const ui32 type_idx = Type_index<Event_base>::get<t_event_type>();

		while (type_idx >= m_engine_events.size())
			m_engine_events.emplace_back();

		auto& engine_event_base = m_engine_events[type_idx];

		if (!engine_event_base)
			engine_event_base = std::make_unique<t_event_type>();

		t_event_type* engine_event = reinterpret_cast<t_event_type*>(engine_event_base.get());

		engine_event->m_listeners.push_back(in_func);
	}

	template<typename t_event_type, typename event_data>
	inline void Engine::invoke(event_data& event_data_to_send)
	{
		const ui32 type_idx = Type_index<Event_base>::get<t_event_type>();

		if (type_idx >= m_engine_events.size())
			return;

		auto& engine_event_base = m_engine_events[type_idx];

		if (!engine_event_base)
			return;

		t_event_type* engine_event = reinterpret_cast<t_event_type*>(engine_event_base.get());

		engine_event->invoke(Engine_context(*this), event_data_to_send);
	}
}
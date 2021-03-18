#pragma once
#include "sic/core/type_restrictions.h"
#include "sic/core/type_index.h"

#include "sic/core/component.h"
#include "sic/core/object_base.h"

#include "sic/core/system.h"
#include "sic/core/event.h"

#include "sic/core/threadpool.h"
#include "sic/core/thread_context.h"
#include "sic/core/engine_job_scheduling.h"

#include <new>
#include <chrono>
#include <unordered_map>
#include <queue>
#include <typeindex>

namespace sic
{
	namespace rtti { struct Typeinfo; struct Rtti; };

	struct Job_dependency
	{
		struct Info
		{
			enum struct Access_type
			{
				read,
				write,
				single_access
			};

			Info(i32 in_type_index, size_t in_index, Access_type in_access_type) : m_type_index(in_type_index), m_index(in_index), m_access_type(in_access_type) {}
			i32 m_type_index = -1;
			size_t m_index = 0;
			Access_type m_access_type = Access_type::write;

			bool m_ready_to_execute = false;
		};

		std::vector<Info> m_infos;
	};

	struct Scene;

	//enginewide state data
	struct State : Noncopyable
	{
	};

	struct Engine_tick_profiling_data
	{
		struct Thread_data
		{
			struct Item
			{
				std::string m_key;
				float m_start = 0.0f;
				float m_end = 0.0f;
			};

			std::string m_name;
			std::vector<Item> m_items;
			float m_idle_time = 0.0f;
		};

		std::vector<Thread_data> m_thread_datas;
		float m_total_time = 0.0f;
	};

	struct Engine : Noncopyable
	{
		struct Type_schedule
		{
			struct Item
			{
				std::function<void()> m_job;
				Job_id m_id;
				std::optional<Job_id> m_job_dependency;
			};

			std::vector<Item> m_read_jobs;
			std::vector<Item> m_write_jobs;
			std::vector<Item> m_single_access_jobs;
			
			std::string m_typename;
		};

		friend struct Engine_context;
		friend struct Scene_context;

		template <typename ...T_processor_flags>
		friend struct Processor_shared;

		friend struct Schedule_for_type;

		Engine();

		void finalize();

		void simulate();
		void prepare_threadpool();
		void tick();
		void on_shutdown();

		void shutdown() { m_is_shutting_down = true; }
		bool is_shutting_down() const { return m_is_shutting_down; }

		template <typename T_system_type>
		void add_system();

		template <typename T_system_type>
		T_system_type& create_system();

		template <typename T_system_type>
		T_system_type& create_system_internal();

		template <typename T_component_type>
		typename T_component_type& create_component(Object_base& in_object_to_attach_to);

		template <typename T_component_type>
		void destroy_component(T_component_type& in_component_to_destroy);

		template <typename T_state>
		T_state* get_state();

		template <typename T_state>
		const T_state* get_state() const;

		void create_scene(Scene* in_parent_scene, std::function<void(Scene_context)> in_on_created_callback = {});
		void destroy_scene(Scene& inout_scene);

		template <typename T_event_type, typename T_functor>
		void listen(T_functor in_func);

		template <typename T_event_type, typename T_event_data>
		void invoke(T_event_data event_data_to_send);

		void refresh_time_delta();
		void flush_scene_streaming();

	private:
		void tick_systems(std::vector<System*>& inout_systems);
		size_t execute_scheduled_jobs();
		void flush_deferred_updates();

		std::unique_ptr<State>& get_state_at_index(i32 in_index);
		void destroy_scene_internal(Scene& in_scene);

		std::vector<std::unique_ptr<System>> m_systems;
		std::vector<std::unique_ptr<State>> m_states;
		
		std::vector<std::unique_ptr<Scene>> m_scenes;
		std::vector<std::unique_ptr<Scene>> m_scenes_to_add;
		std::vector<i32> m_scenes_to_remove;
		std::unordered_map<i32, Scene*> m_scene_id_to_scene_lut;
		i32 m_scene_id_ticker = 0;

		std::vector<std::unique_ptr<Event_base>> m_engine_events;

		std::unique_ptr<rtti::Rtti> m_rtti;
		
		std::vector<System*> m_tick_systems;

		std::unordered_map<i32, Type_schedule> m_type_index_to_schedule;
		std::unordered_map<i32, Job_dependency> m_job_id_to_type_dependencies_lut;
		std::vector<Type_schedule::Item> m_no_flag_jobs;

		Main_thread_worker m_main_thread_worker;
		std::vector<Job_node> m_job_nodes;
		std::vector<Job_node*> m_job_leaf_nodes;

		std::vector<Thread_context*> m_thread_contexts;

		Threadpool m_system_ticker_threadpool;

		std::chrono::time_point<std::chrono::high_resolution_clock> m_previous_frame_time_point;
		float m_time_delta = -1.0f;
		sic::i64 m_current_frame = 0;

		bool m_is_shutting_down = true;
		bool m_initialized = false;
		bool m_has_prepared_threadpool = false;
		bool m_finished_setup = false;
		std::mutex m_scenes_mutex;

		//callbacks to run whenever a new scene is created
		std::vector<std::function<void(Scene&)>> m_registration_callbacks;

		public:
		Engine_tick_profiling_data m_tick_profiling_data;
		bool m_is_profiling = true;
	};

	template<typename T_system_type>
	inline void Engine::add_system()
	{
		m_tick_systems.push_back(&create_system<T_system_type>());
	}

	template<typename T_system_type>
	inline T_system_type& Engine::create_system()
	{
		T_system_type& sys = create_system_internal<T_system_type>();

		return sys;
	}

	template<typename T_system_type>
	inline T_system_type& Engine::create_system_internal()
	{
		static_assert(std::is_base_of<System, T_system_type>::value, "system_type must derive from struct System");

		const ui64 new_system_idx = m_systems.size();

		m_systems.push_back(std::move(std::make_unique<T_system_type>()));
		
		m_systems.back()->m_name = typeid(T_system_type).name();
		m_systems.back()->on_created(std::move(Engine_context(*this)));
		
		return *reinterpret_cast<T_system_type*>(m_systems[new_system_idx].get());
	}

	template<typename T_component_type>
	inline typename T_component_type& Engine::create_component(Object_base& in_object_to_attach_to)
	{
		const ui32 type_idx = Type_index<Component_base>::get<T_component_type>();

		assert(type_idx < m_component_storages.size() && m_component_storages[type_idx]->initialized() && "component was not registered before use");

		Component_storage<T_component_type>* storage = reinterpret_cast<Component_storage<T_component_type>*>(m_component_storages[type_idx].get());
		auto& comp = storage->create_component();

		comp.m_owner = &in_object_to_attach_to;

		invoke<Event_created<T_component_type>>(comp);

		return comp;
	}

	template<typename T_component_type>
	inline void Engine::destroy_component(T_component_type& in_component_to_destroy)
	{
		const ui32 type_idx = Type_index<Component_base>::get<T_component_type>();
		invoke<Event_destroyed<T_component_type>>(in_component_to_destroy);

		Component_storage<T_component_type>* storage = reinterpret_cast<Component_storage<T_component_type>*>(m_component_storages[type_idx].get());
		storage->destroy_component(in_component_to_destroy);
	}

	template<typename T_state>
	inline T_state* Engine::get_state()
	{
		constexpr bool is_valid_type = std::is_base_of<State, T_state>::value;

		static_assert(is_valid_type, "can only get types that derive State");

		const i32 type_idx = Type_index<State>::get<T_state>();

		assert((type_idx < m_states.size() && m_states[type_idx].get() != nullptr) && "state not registered");

		return reinterpret_cast<T_state*>(m_states[type_idx].get());
	}

	template<typename T_state>
	inline const T_state* Engine::get_state() const
	{
		constexpr bool is_valid_type = std::is_base_of<State, T_state>::value;

		static_assert(is_valid_type, "can only get types that derive State");

		const i32 type_idx = Type_index<State>::get<T_state>();

		assert((type_idx < m_states.size() && m_states[type_idx].get() != nullptr) && "state not registered");

		return reinterpret_cast<const T_state*>(m_states[type_idx].get());
	}

	template<typename T_event_type, typename T_functor>
	inline void Engine::listen(T_functor in_func)
	{
		const ui32 type_idx = Type_index<Event_base>::get<T_event_type>();

		while (type_idx >= m_engine_events.size())
			m_engine_events.emplace_back();

		auto& engine_event_base = m_engine_events[type_idx];

		if (!engine_event_base)
			engine_event_base = std::make_unique<T_event_type>();

		T_event_type* engine_event = reinterpret_cast<T_event_type*>(engine_event_base.get());

		engine_event->m_listeners.push_back(in_func);
	}

	template<typename T_event_type, typename T_event_data>
	inline void Engine::invoke(T_event_data event_data_to_send)
	{
		const ui32 type_idx = Type_index<Event_base>::get<T_event_type>();

		if (type_idx >= m_engine_events.size())
			return;

		auto& engine_event_base = m_engine_events[type_idx];

		if (!engine_event_base)
			return;

		T_event_type* engine_event = reinterpret_cast<T_event_type*>(engine_event_base.get());


		engine_event->invoke(Engine_context(*this), event_data_to_send);
	}
}
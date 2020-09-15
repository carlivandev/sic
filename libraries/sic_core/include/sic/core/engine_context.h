#pragma once
#include "engine.h"
#include "level.h"

namespace sic
{
	struct Schedule_data
	{
		Schedule_data& job_dependency(Job_id in_job_id) { m_job_dependency = in_job_id; return *this; }
		Schedule_data& finish_before_tickstep(Tickstep in_tickstep) { m_finish_before_tickstep = in_tickstep; return *this; }
		Schedule_data& run_on_main_thread(bool in_run_on_main_thread) { m_run_on_main_thread = in_run_on_main_thread; return *this; }

		std::optional<Job_id> m_job_dependency;
		std::optional<Tickstep> m_finish_before_tickstep;
		bool m_run_on_main_thread = false;
	};

	struct Engine_context
	{
		template <typename ...T_processor_flags>
		friend struct Processor;

		Engine_context() = default;
		Engine_context(Engine& in_engine) : m_engine(&in_engine) {}

		template<typename T_system_type>
		__forceinline T_system_type& create_subsystem(System& inout_system)
		{
			auto& new_system = m_engine->create_system<T_system_type>();
			inout_system.m_subsystems.push_back(&new_system);

			return new_system;
		}

		template <typename T_component>
		__forceinline constexpr Type_reg register_component_type(const char* in_unique_key, ui32 in_initial_capacity = 128)
		{
			return m_engine->register_component_type<T_component>(in_unique_key, in_initial_capacity);
		}

		template <typename T_object>
		__forceinline constexpr Type_reg register_object(const char* in_unique_key, ui32 in_initial_capacity = 128, ui32 in_bucket_capacity = 64)
		{
			return m_engine->register_object<T_object>(in_unique_key, in_initial_capacity, in_bucket_capacity);
		}

		template <typename T_state>
		__forceinline constexpr Type_reg register_state(const char* in_unique_key)
		{
			const ui32 type_idx = Type_index<State>::get<T_state>();

			auto& state_to_register = m_engine->get_state_at_index(type_idx);

			assert(state_to_register == nullptr && "state already registered!");

			state_to_register = std::make_unique<T_state>();

			return std::move(register_typeinfo<T_state>(in_unique_key));
		}

		template <typename T_type_to_register>
		__forceinline constexpr Type_reg register_typeinfo(const char* in_unique_key)
		{
			return m_engine->register_typeinfo<T_type_to_register>(in_unique_key);
		}

		template <typename T_type>
		__forceinline constexpr Typeinfo* get_typeinfo()
		{
			return m_engine->get_typeinfo<T_type>();
		}

		template <typename T_state>
		__forceinline T_state* get_state()
		{
			return m_engine->get_state<T_state>();
		}

		template <typename T_state>
		__forceinline T_state& get_state_checked()
		{
			T_state* state = m_engine->get_state<T_state>();
			assert(state && "State was not registered before use!");

			return *state;
		}

		template <typename T_state>
		__forceinline const T_state& get_state_checked() const
		{
			const T_state* state = m_engine->get_state<T_state>();
			assert(state && "State was not registered before use!");

			return *state;
		}

		template <typename T_event_type, typename T_functor>
		void listen(T_functor in_func)
		{
			m_engine->listen<T_event_type, T_functor>(std::move(in_func));
		}

		template <typename T_event_type, typename T_event_data>
		void invoke(T_event_data& event_data_to_send)
		{
			m_engine->invoke<T_event_type, T_event_data>(event_data_to_send);
		}

		template<typename T_type>
		__forceinline void for_each_w(std::function<void(T_type&)> in_func)
		{
			if constexpr (std::is_same<T_type, Scene>::value)
			{
				for (auto& level : m_engine->m_levels)
					for_each_level(in_func, *level.get());
			}
			else
			{
				for (auto& level : m_engine->m_levels)
					level->for_each_w<T_type>(in_func);
			}
		}

		template<typename T_type>
		__forceinline void for_each_r(std::function<void(const T_type&)> in_func) const
		{
			if constexpr (std::is_same<T_type, Scene>::value)
			{
				for (auto& level : m_engine->m_levels)
					for_each_level(in_func, *level.get());
			}
			else
			{
				for (auto& level : m_engine->m_levels)
					level->for_each_r<T_type>(in_func);
			}
		}

		template<typename T_type>
		__forceinline T_type* find_w(Object_base& inout_object)
		{
			return inout_object.find<Component_transform>();
		}

		template<typename T_type>
		__forceinline const T_type* find_r(const Object_base& in_object) const
		{
			return in_object.find<Component_transform>();
		}

		template <typename ...T>
		__forceinline Job_id schedule(void (*in_job)(Processor<T...>), Schedule_data in_data = Schedule_data())
		{
			Job_id job_id;
			job_id.m_id = m_engine->m_job_index_ticker++;
			job_id.m_run_on_main_thread = in_data.m_run_on_main_thread;

			if (in_data.m_job_dependency.has_value())
				job_id.m_job_dependency = in_data.m_job_dependency->m_id;

			Engine_context context(*m_engine);
			auto job_callback =
				[in_job, context]()
			{
				Processor<T...> processor(context);
				in_job(processor);
			};

			auto& dependency_infos = m_engine->m_job_id_to_type_dependencies_lut[job_id.m_id];
			(Processor<T...>::schedule_for_type<T>(*m_engine, dependency_infos, job_callback, job_id), ...);

			return job_id;
		}

		void create_level(Scene* in_parent_level)
		{
			m_engine->create_level(in_parent_level);
		}

		void destroy_level(Scene& inout_level)
		{
			m_engine->destroy_level(inout_level);
		}

		void shutdown()
		{
			m_engine->shutdown();
		}

		bool get_does_object_exist(Object_id in_id) const
		{
			std::scoped_lock lock(m_engine->m_levels_mutex);

			auto it = m_engine->m_level_id_to_level_lut.find(in_id.get_level_id());

			if (it == m_engine->m_level_id_to_level_lut.end())
				return false;

			return it->second->get_does_object_exist(in_id, false);
		}

		float get_time_delta() const
		{
			return m_engine->m_time_delta;
		}

	private:
		void for_each_level(std::function<void(Scene&)> in_func, Scene& inout_level)
		{
			in_func(inout_level);

			for (auto& sublevel : inout_level.m_sublevels)
				for_each_level(in_func, *sublevel.get());
		}

		Engine* m_engine = nullptr;
	};
}


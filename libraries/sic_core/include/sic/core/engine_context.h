#pragma once
#include "sic/core/engine.h"
#include "sic/core/scene.h"

namespace sic
{
	template <typename T_return_type, typename ...T_args>
	std::function<T_return_type(T_args...)> make_functor(T_return_type(*in_function_pointer)(T_args...))
	{
		return std::function<T_return_type(T_args...)>(in_function_pointer);
	}

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
		__forceinline constexpr void register_component_type(const char* in_unique_key, ui32 in_initial_capacity = 128)
		{
			m_engine->register_component_type<T_component>(in_unique_key, in_initial_capacity);
		}

		template <typename T_object>
		__forceinline constexpr void register_object(const char* in_unique_key, ui32 in_initial_capacity = 128, ui32 in_bucket_capacity = 64)
		{
			m_engine->register_object<T_object>(in_unique_key, in_initial_capacity, in_bucket_capacity);
		}

		template <typename T_state>
		__forceinline constexpr void register_state(const char* in_unique_key)
		{
			const ui32 type_idx = Type_index<State>::get<T_state>();

			auto& state_to_register = m_engine->get_state_at_index(type_idx);

			assert(state_to_register == nullptr && "state already registered!");

			state_to_register = std::make_unique<T_state>();

			register_typeinfo<T_state>(in_unique_key);
		}

		template <typename T_type_to_register>
		__forceinline constexpr void register_typeinfo(const char* in_unique_key)
		{
			m_engine->register_typeinfo<T_type_to_register>(in_unique_key);
		}

		template <typename T_type>
		__forceinline constexpr const rtti::Typeinfo* get_typeinfo() const
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
		void invoke(T_event_data event_data_to_send)
		{
			this_thread().update_deferred
			(
				[engine = m_engine, event_data_to_send](Engine_context)
				{
					engine->invoke<T_event_type, T_event_data>(event_data_to_send);
				}
			);
		}

		//NOT THREAD SAFE!
		template <typename T_event_type, typename T_event_data>
		void invoke_immediate(T_event_data& event_data_to_send)
		{
			m_engine->invoke<T_event_type, T_event_data>(event_data_to_send);
		}

		template<typename T_type>
		__forceinline void for_each_w(std::function<void(T_type&)> in_func)
		{
			if constexpr (std::is_same<T_type, Scene>::value)
			{
				for (auto& scene : m_engine->m_scenes)
					for_each_scene(in_func, *scene.get());
			}
			else
			{
				for (auto& scene : m_engine->m_scenes)
					scene->for_each_w<T_type>(in_func);
			}
		}

		template<typename T_type>
		__forceinline void for_each_r(std::function<void(const T_type&)> in_func) const
		{
			if constexpr (std::is_same<T_type, Scene>::value)
			{
				for (auto& scene : m_engine->m_scenes)
					for_each_scene(in_func, *scene.get());
			}
			else
			{
				for (auto& scene : m_engine->m_scenes)
					scene->for_each_r<T_type>(in_func);
			}
		}

		template<typename T_type>
		__forceinline T_type* find_w(Object_base& inout_object)
		{
			return inout_object.find<T_type>();
		}

		template<typename T_type>
		__forceinline const T_type* find_r(const Object_base& in_object) const
		{
			return in_object.find<T_type>();
		}

		template<typename T_type, typename T_object_type>
		__forceinline T_type& get_w(T_object_type& inout_object)
		{
			return inout_object.get<T_type>();
		}

		template<typename T_type, typename T_object_type>
		__forceinline const T_type& get_r(const T_object_type& in_object) const
		{
			return in_object.get<T_type>();
		}

		template <typename ...T>
		__forceinline Job_id schedule(std::function<void(Processor<T...>)> in_job, Schedule_data in_data = Schedule_data())
		{
			auto& thread_this = this_thread();

			Job_id job_id;
			job_id.m_id = thread_this.m_job_id_ticker++;
			job_id.m_submitted_on_thread_id = thread_this.get_id();
			job_id.m_run_on_main_thread = in_data.m_run_on_main_thread;

			if (in_data.m_job_dependency.has_value())
			{
				job_id.m_job_dependency.emplace();
				job_id.m_job_dependency.value().m_id = in_data.m_job_dependency->m_id;
				job_id.m_job_dependency.value().m_submitted_on_thread_id = in_data.m_job_dependency->m_submitted_on_thread_id;
			}

			thread_this.m_scheduled_items.push_back
			(
				{
					job_id,
					[in_job](Engine_context in_context, Job_id in_job_id)
					{
						auto job_callback =
							[in_job, in_context]()
						{
							Processor<T...> processor(in_context);
							in_job(processor);
						};

						
						const size_t job_index_offset = in_context.m_engine->m_thread_contexts[in_job_id.m_submitted_on_thread_id]->m_job_index_offset;
						auto& dependency_infos = in_context.m_engine->m_job_id_to_type_dependencies_lut[job_index_offset + in_job_id.m_id];
						(Processor<T...>::schedule_for_type<T>(*in_context.m_engine, dependency_infos, job_callback, in_job_id), ...);
					}
				}
			);

			return job_id;
		}

		template <typename ...T>
		__forceinline void schedule_timed(std::function<void(Processor<T...>)> in_job, float in_duration, float in_delay_until_start, bool in_run_on_main_thread = false)
		{
			auto& thread_this = this_thread();

			Job_id job_id;
			job_id.m_id = -1;
			job_id.m_submitted_on_thread_id = thread_this.get_id();
			job_id.m_run_on_main_thread = in_run_on_main_thread;

			Thread_context::Timed_scheduled_item* new_item = nullptr;

			if (thread_this.m_free_timed_scheduled_items_indices.empty())
			{
				new_item = &thread_this.m_timed_scheduled_items.emplace_back();
			}
			else
			{
				new_item = &thread_this.m_timed_scheduled_items[thread_this.m_free_timed_scheduled_items_indices.back()];
				thread_this.m_free_timed_scheduled_items_indices.pop_back();
			}

			*new_item =
			Thread_context::Timed_scheduled_item
			{
				[job_id, in_job, &thread_this](Engine_context in_context) mutable
				{
					auto job_callback = [in_job, in_context]()
					{
						Processor<T...> processor(in_context);
						in_job(processor);
					};

					job_id.m_id = thread_this.m_job_id_ticker++;

					auto& dependency_infos = in_context.m_engine->m_job_id_to_type_dependencies_lut[thread_this.m_job_index_offset + job_id.m_id];
					(Processor<T...>::schedule_for_type<T>(*in_context.m_engine, dependency_infos, job_callback, job_id), ...);
				},
				in_duration,
				in_delay_until_start
			};
		}

		void create_scene(Scene* in_parent_scene)
		{
			m_engine->create_scene(in_parent_scene);
		}

		void destroy_scene(Scene& inout_scene)
		{
			m_engine->destroy_scene(inout_scene);
		}

		Scene* find_scene(sic::i32 in_scene_index)
		{
			if (in_scene_index >= 0 && in_scene_index < m_engine->m_scenes.size())
				return m_engine->m_scenes[in_scene_index].get();

			return nullptr;
		}

		void shutdown()
		{
			m_engine->shutdown();
		}

		bool get_does_object_exist(Object_id in_id) const
		{
			std::scoped_lock lock(m_engine->m_scenes_mutex);

			auto it = m_engine->m_scene_id_to_scene_lut.find(in_id.get_scene_id());

			if (it == m_engine->m_scene_id_to_scene_lut.end())
				return false;

			return it->second->get_does_object_exist(in_id, false);
		}

		float get_time_delta() const
		{
			return m_engine->m_time_delta;
		}

	private:
		void for_each_scene(std::function<void(Scene&)> in_func, Scene& inout_scene)
		{
			in_func(inout_scene);

			for (auto& subscene : inout_scene.m_subscenes)
				for_each_scene(in_func, *subscene.get());
		}

		Engine* m_engine = nullptr;
	};
}


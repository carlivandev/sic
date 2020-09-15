#pragma once
#include "sic/core/level.h"
#include "sic/core/bucket_allocator_view.h"
#include "sic/core/object.h"
#include "sic/core/engine_context.h"

#include <optional>

namespace sic
{
	struct Processor_flag_base {};

	struct Processor_flag_read_base : Processor_flag_base {};
	template <typename T>
	struct Processor_flag_read : Processor_flag_read_base {};

	struct Processor_flag_write_base : Processor_flag_base {};
	template <typename T>
	struct Processor_flag_write : Processor_flag_write_base {};

	struct Processor_flag_deferred_write_base : Processor_flag_base {};
	template <typename T>
	struct Processor_flag_deferred_write : Processor_flag_deferred_write_base {};

	struct Processor_base {};
	template <typename ...T_processor_flags>
	struct Processor : Processor_base
	{
		friend struct Scene_context;
		friend Engine_context;

		explicit Processor(Scene_context in_context) : m_engine(&in_context.m_engine), m_scene(&in_context.m_level) {}
		explicit Processor(Engine_context in_context) : m_engine(in_context.m_engine), m_scene(nullptr) {}

		template<typename T_type>
		__forceinline void for_each_w(std::function<void(T_type&)> in_func)
		{
			static_assert((std::is_same<T_processor_flags, Processor_flag_write<T_type>>::value || ...), "Processor does not have write flag for T_type");
			
			if (m_scene)
				m_scene->for_each_w<T_type>(in_func);
			else
				Engine_context(*m_engine).for_each_w<T_type>(in_func);
		}

		template<typename T_type>
		__forceinline void for_each_r(std::function<void(const T_type&)> in_func) const
		{
			static_assert
			(
				(std::is_same<T_processor_flags, Processor_flag_read<T_type>>::value || ...) ||
				(std::is_same<T_processor_flags, Processor_flag_write<T_type>>::value || ...)
				, "Processor does not have read/write flag for T_type"
			);
			
			if (m_scene)
				m_scene->for_each_r<T_type>(in_func);
			else
				Engine_context(*m_engine).for_each_r<T_type>(in_func);
		}

		template<typename T_type>
		__forceinline T_type& get_state_checked_w()
		{
			static_assert((std::is_same<T_processor_flags, Processor_flag_write<T_type>>::value || ...), "Processor does not have write flag for T_type");

			return Engine_context(*m_engine).get_state_checked<T_type>();
		}

		template<typename T_type>
		__forceinline const T_type& get_state_checked_r() const
		{
			static_assert
			(
				(std::is_same<T_processor_flags, Processor_flag_read<T_type>>::value || ...) ||
				(std::is_same<T_processor_flags, Processor_flag_write<T_type>>::value || ...)
				, "Processor does not have read/write flag for T_type"
			);

			return Engine_context(*m_engine).get_state_checked<T_type>();
		}

		template<typename T_type>
		__forceinline T_type* get_state_w()
		{
			static_assert((std::is_same<T_processor_flags, Processor_flag_write<T_type>>::value || ...), "Processor does not have write flag for T_type");

			return Engine_context(*m_engine).get_state<T_type>();
		}

		template<typename T_type>
		__forceinline const T_type* get_state_r() const
		{
			static_assert
				(
					(std::is_same<T_processor_flags, Processor_flag_read<T_type>>::value || ...) ||
					(std::is_same<T_processor_flags, Processor_flag_write<T_type>>::value || ...)
					, "Processor does not have read/write flag for T_type"
					);

			return Engine_context(*m_engine).get_state<T_type>();
		}

		template<typename T_type>
		__forceinline void update_state_deferred(std::function<void(T_type&)> in_func) const
		{
			static_assert
			(
				(std::is_same<T_processor_flags, Processor_flag_deferred_write<T_type>>::value || ...)
				, "Processor does not have deferred write flag for T_type"
			);

			auto update_func =
			[in_func](Engine_context in_context)
			{
				T_type* state = in_context.get_state<T_type>();

				if (state)
					in_func(*state);
			};

			this_thread().update_deferred(update_func);
		}

		template <typename T_processor_type>
		operator T_processor_type() const
		{
			static_assert
			(
				(std::is_same<T_processor_flags, T_processor_type>::value || ...)
				, "Processor can only be converted if it has T_processor_type as part of its own <T...>"
			);

			return *((const T_processor_type*)((const void*)(this)));
		}

		float get_time_delta() const
		{
			return Engine_context(*m_engine).get_time_delta();
		}

		//carl TODO: these should be private when Processors are finished
		Engine* m_engine = nullptr;
		Scene* m_scene = nullptr;

		template <typename T>
		static void schedule_for_type(Engine& inout_engine, Job_dependency& inout_dependency_infos, std::function<void()> in_callback, Job_id in_id)
		{
			const i32 type_idx = Type_index<Processor_flag_base>::get<T>();

			if constexpr (std::is_base_of<Processor_flag_read_base, T>::value)
			{
				Engine::Type_schedule::Item new_schedule_item;
				new_schedule_item.m_job = in_callback;
				new_schedule_item.m_id = in_id;

				Engine::Type_schedule& schedule_for_type = inout_engine.m_type_index_to_schedule[type_idx];
				schedule_for_type.m_typename = typeid(T).name();
				inout_dependency_infos.m_infos.push_back(Job_dependency::Info(type_idx, schedule_for_type.m_read_jobs.size(), true));
				schedule_for_type.m_read_jobs.push_back(new_schedule_item);
			}
			else if constexpr (std::is_base_of<Processor_flag_write_base, T>::value)
			{
				Engine::Type_schedule::Item new_schedule_item;
				new_schedule_item.m_job = in_callback;
				new_schedule_item.m_id = in_id;

				Engine::Type_schedule& schedule_for_type = inout_engine.m_type_index_to_schedule[type_idx];
				schedule_for_type.m_typename = typeid(T).name();
				inout_dependency_infos.m_infos.push_back(Job_dependency::Info(type_idx, schedule_for_type.m_write_jobs.size(), false));
				schedule_for_type.m_write_jobs.push_back(new_schedule_item);
			}
			else if constexpr (std::is_base_of<Processor_flag_deferred_write_base, T>::value)
			{
				Engine::Type_schedule::Item new_schedule_item;
				new_schedule_item.m_job = in_callback;
				new_schedule_item.m_id = in_id;

				Engine::Type_schedule& schedule_for_type = inout_engine.m_type_index_to_schedule[type_idx];
				schedule_for_type.m_typename = typeid(T).name();
				inout_dependency_infos.m_infos.push_back(Job_dependency::Info(type_idx, schedule_for_type.m_deferred_write_jobs.size(), true)); //consider it a read, since we dont want it to have write dependencies
				schedule_for_type.m_deferred_write_jobs.push_back(new_schedule_item);
			}
			else if constexpr (std::is_base_of<Processor_base, T>::value)
			{
				T::schedule_for_types(inout_engine, inout_dependency_infos, in_callback, in_id);
			}
			else
			{
				static_assert(false, "Not an accepted processor flag!");
			}
		}

		__forceinline static void schedule_for_types(Engine& inout_engine, Job_dependency& inout_dependency_infos, std::function<void()> in_callback, Job_id in_id)
		{
			(schedule_for_type<T_processor_flags>(inout_engine, inout_dependency_infos, in_callback, in_id), ...);
		}
	};

	struct Object_base;
	
	struct Scene_context
	{
		template <typename T_subtype, typename ...T_component>
		friend struct Object;

		template <typename ...T_processor_flags>
		friend struct Processor;

		Scene_context(Engine& inout_engine, Scene& inout_level) : m_engine(inout_engine), m_level(inout_level){}

		template <typename T_object>
		__forceinline constexpr T_object& create_object()
		{
			return m_level.create_object<T_object>();
		}

		void destroy_object(Object_base& in_object_to_destroy)
		{
			m_level.destroy_object(in_object_to_destroy);
		}

		template<typename T_type>
		__forceinline void for_each_w(std::function<void(T_type&)> in_func)
		{
			m_level.for_each_w<T_type>(in_func);
		}

		template<typename T_type>
		__forceinline void for_each_r(std::function<void(const T_type&)> in_func) const
		{
			m_level.for_each_w<T_type>(in_func);
		}

		template <typename ...T>
		Job_id schedule(void (*in_job)(Processor<T...>), std::optional<Job_id> in_job_dependency = {}, std::optional<Tickstep> in_finish_before_tickstep = {}, bool in_run_on_main_thread = false)
		{
			Job_id job_id;
			job_id.m_id = m_engine.m_job_index_ticker++;
			job_id.m_run_on_main_thread = in_run_on_main_thread;

			if (in_job_dependency.has_value())
				job_id.m_job_dependency = in_job_dependency->m_id;

			Scene_context context(m_engine, m_level);
			auto job_callback =
			[in_job, context]()
			{
				Processor<T...> processor(context);
				in_job(processor);
			};

			auto& dependency_infos = m_engine.m_job_id_to_type_dependencies_lut[job_id.m_id];
			(Processor<T...>::schedule_for_type<T>(m_engine, dependency_infos, job_callback, job_id), ...);

			return job_id;
		}

		Engine_context get_engine_context() { return m_engine; }
		const Engine_context get_engine_context() const { return m_engine; }
		
		i32 get_outermost_level_id() const { return m_level.m_outermost_level != nullptr ? m_level.m_outermost_level->m_level_id : get_level_id(); }
		i32 get_level_id() const { return m_level.m_level_id; }
		bool get_does_object_exist(Object_id in_id, bool in_only_in_this_level) const { return m_level.get_does_object_exist(in_id, in_only_in_this_level); }

	private:
		Engine& m_engine;
		Scene& m_level;
	};
}


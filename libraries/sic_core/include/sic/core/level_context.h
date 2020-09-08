#pragma once
#include "sic/core/level.h"
#include "sic/core/bucket_allocator_view.h"
#include "sic/core/object.h"
#include "sic/core/engine_context.h"

namespace sic
{
	struct Processor_flag_read_base {};
	template <typename T>
	struct Processor_flag_read : Processor_flag_read_base {};

	struct Processor_flag_write_base {};
	template <typename T>
	struct Processor_flag_write : Processor_flag_write_base {};

	template <typename ...T_processor_flags>
	struct Processor
	{
		Processor(Scene_context& in_context) : m_context(in_context) {}

		template<typename T_type>
		__forceinline void for_each_w(std::function<void(T_type&)> in_func)
		{
			static_assert((std::is_same<T_processor_flags, Processor_flag_write<T_type>>::value, ...), "Processor does not have write flag for T_type");
			m_context.for_each_w<T_type>(in_func);
		}

		template<typename T_type>
		__forceinline void for_each_r(std::function<void(const T_type&)> in_func) const
		{
			static_assert
			(
				(std::is_same<T_processor_flags, Processor_flag_read<T_type>>::value, ...) ||
				(std::is_same<T_processor_flags, Processor_flag_write<T_type>>::value, ...)
				, "Processor does not have read/write flag for T_type"
			);
			m_context.for_each_r<T_type>(in_func);
		}

		Scene_context& m_context;
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

		template <typename ...T_processor_flags>
		Processor<T_processor_flags...> process()
		{
			return Processor<T_processor_flags...>(*this);
		}

		template <typename T>
		void schedule_for_type(Job_dependency& inout_dependency_infos, std::function<void()> in_callback, Job_id in_id)
		{
			Engine::Type_schedule& schedule_for_type = m_engine.m_type_to_schedule[typeid(T)];
			Engine::Type_schedule::Item new_schedule_item;
			new_schedule_item.m_job = in_callback;
			new_schedule_item.m_id = in_id;

			if constexpr (std::is_base_of<Processor_flag_read_base, T>::value)
			{
				inout_dependency_infos.m_infos.push_back(Job_dependency::Info(typeid(T), schedule_for_type.m_read_jobs.size(), true));
				schedule_for_type.m_read_jobs.push(new_schedule_item);
			}
			else if constexpr (std::is_base_of<Processor_flag_write_base, T>::value)
			{
				inout_dependency_infos.m_infos.push_back(Job_dependency::Info(typeid(T), schedule_for_type.m_write_jobs.size(), false));
				schedule_for_type.m_write_jobs.push(new_schedule_item);
			}
			else
			{
				static_assert(false, "Not an accepted processor flag!");
			}
		}

		template <typename ...T>
		Job_id schedule(void (*in_job)(Processor<T...>), std::optional<Job_id> in_job_dependency = {}, std::optional<Tickstep> in_finish_before_tickstep = {}, std::optional<std::string> in_thread_name = {})
		{
			Job_id job_id;
			job_id.m_id = m_engine.m_job_index_ticker++;

			auto job_callback =
			[in_job, this]()
			{
				in_job(Processor<T...>(*this));
// 				auto& dependency_infos = m_engine.m_job_id_to_dependencies_lut[job_id.m_id];
// 				dependency_infos.m_infos
			};

			auto& dependency_infos = m_engine.m_job_id_to_dependencies_lut[job_id.m_id];
			(schedule_for_type<T>(dependency_infos, job_callback, job_id), ...);

			return job_id;
		}

		Engine_context get_engine_context() { return m_engine; }
		const Engine_context get_engine_context() const { return m_engine; }
		
		i32 get_outermost_level_id() const { return m_level.m_outermost_level != nullptr ? m_level.m_outermost_level->m_level_id : get_level_id(); }
		i32 get_level_id() const { return m_level.m_level_id; }
		bool get_does_object_exist(Object_id in_id, bool in_only_in_this_level) const { return m_level.get_does_object_exist(in_id, in_only_in_this_level); }

	private:
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

		Engine& m_engine;
		Scene& m_level;
	};
}


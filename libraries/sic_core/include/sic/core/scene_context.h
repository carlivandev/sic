#pragma once
#include "sic/core/scene.h"
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

	struct Processor_flag_access_single_base : Processor_flag_base {};
	template <typename T>
	struct Processor_flag_read_single : Processor_flag_access_single_base {};

	template <typename T>
	struct Processor_flag_write_single : Processor_flag_access_single_base {};

	template <typename T>
	using Pf_read = Processor_flag_read<T>;

	template <typename T>
	using Pf_write = Processor_flag_write<T>;

	template <typename T>
	using Pf_read_1 = Processor_flag_read_single<T>;

	template <typename T>
	using Pf_write_1 = Processor_flag_write_single<T>;

	struct Processor_base {};

	template <typename ...T_processor_flags>
	struct Processor_shared : Processor_base
	{
		explicit Processor_shared(Engine* in_engine) : m_engine(in_engine) {}

		template<typename T_type>
		__forceinline T_type& get_state_checked_w()
		{
			static_assert(verify_flags<Processor_flag_write<T_type>>(), "Processor does not have write flag for T_type");

			return Engine_context(*m_engine).get_state_checked<T_type>();
		}

		template<typename T_type>
		__forceinline const T_type& get_state_checked_r() const
		{
			static_assert
			(
				verify_flags<Processor_flag_read<T_type>>() ||
				verify_flags<Processor_flag_write<T_type>>()
				, "Processor does not have read/write flag for T_type"
			);

			return Engine_context(*m_engine).get_state_checked<T_type>();
		}

		template<typename T_type>
		__forceinline T_type* get_state_w()
		{
			static_assert(verify_flags<Processor_flag_write<T_type>>(), "Processor does not have write flag for T_type");

			return Engine_context(*m_engine).get_state<T_type>();
		}

		template<typename T_type>
		__forceinline const T_type* get_state_r() const
		{
			static_assert
			(
				verify_flags<Processor_flag_read<T_type>>() ||
				verify_flags<Processor_flag_write<T_type>>()
				, "Processor does not have read/write flag for T_type"
			);

			return Engine_context(*m_engine).get_state<T_type>();
		}

		template<typename T_type>
		__forceinline T_type* find_w(Object_base& inout_object)
		{
			static_assert
			(
				verify_flags<Processor_flag_write_single<T_type>>() ||
				verify_flags<Processor_flag_write<T_type>>()
				, "Processor does not have read/write single flag for T_type"
			);

			return Engine_context(*m_engine).find_w<T_type>(inout_object);
		}

		template<typename T_type>
		__forceinline const T_type* find_r(const Object_base& in_object) const
		{
			static_assert
			(
				verify_flags<Processor_flag_read_single<T_type>>() ||
				verify_flags<Processor_flag_write_single<T_type>>() ||
				verify_flags<Processor_flag_read<T_type>>() ||
				verify_flags<Processor_flag_write<T_type>>()
				, "Processor does not have read/write single flag for T_type"
			);

			return Engine_context(*m_engine).find_r<T_type>(in_object);
		}

		template<typename T_type, typename T_object_type>
		__forceinline T_type& get_w(T_object_type& inout_object)
		{
			static_assert
			(
				verify_flags<Processor_flag_write_single<T_type>>() ||
				verify_flags<Processor_flag_write<T_type>>()
				, "Processor does not have read/write single flag for T_type"
			);

			return Engine_context(*m_engine).get_w<T_type>(inout_object);
		}

		template<typename T_type, typename T_object_type>
		__forceinline const T_type& get_r(const T_object_type& in_object) const
		{
			static_assert
			(
				verify_flags<Processor_flag_read_single<T_type>>() ||
				verify_flags<Processor_flag_write_single<T_type>>() ||
				verify_flags<Processor_flag_read<T_type>>() ||
				verify_flags<Processor_flag_write<T_type>>()
				, "Processor does not have read/write single flag for T_type"
			);

			return Engine_context(*m_engine).get_r<T_type>(in_object);
		}

		template <typename T_type>
		__forceinline constexpr const rtti::Typeinfo* find_typeinfo() const
		{
			return Engine_context(*m_engine).find_typeinfo<T_type>();
		}

		const rtti::Rtti& get_rtti() const
		{
			return Engine_context(*m_engine).get_rtti();
		}

		template <typename T_event_type, typename T_event_data>
		void invoke(T_event_data event_data_to_send)
		{
			Engine_context(*m_engine).invoke<T_event_type>(event_data_to_send);
		}

		template<typename T_type>
		__forceinline constexpr static bool verify_flags()
		{
			return (verify_flag<T_processor_flags, T_type>() || ...);
		}

		template<typename T_our_flag_type, typename T_check_flag_type>
		__forceinline constexpr static bool verify_flag()
		{
			if constexpr (std::is_base_of<Processor_base, T_our_flag_type>::value)
			{
				if constexpr (std::is_base_of<Processor_base, T_check_flag_type>::value)
				{
					return std::is_same<T_our_flag_type, T_check_flag_type>::value || T_our_flag_type::verify_flags<T_check_flag_type>();
				}
				else
				{
					return T_our_flag_type::verify_flags<T_check_flag_type>();
				}
			}
			else if constexpr (std::is_base_of<Processor_flag_base, T_our_flag_type>::value)
			{
				return std::is_same<T_our_flag_type, T_check_flag_type>::value;
			}
			else
			{
				static_assert(false, "Not an accepted processor flag!");
			}
		}

		float get_time_delta() const
		{
			return Engine_context(*m_engine).get_time_delta();
		}

		template <typename T>
		static void schedule_for_type(Engine& inout_engine, Job_dependency& inout_dependency_infos, std::function<void()>& in_callback, Job_id in_id)
		{
			const i32 type_idx = Type_index<Processor_flag_base>::get<T>();

			if constexpr (std::is_base_of<Processor_flag_read_base, T>::value)
			{
				Engine::Type_schedule::Item new_schedule_item;
				new_schedule_item.m_job = in_callback;
				new_schedule_item.m_id = in_id;

				Engine::Type_schedule& schedule_for_type = inout_engine.m_type_index_to_schedule[type_idx];
				schedule_for_type.m_typename = typeid(T).name();
				inout_dependency_infos.m_infos.push_back(Job_dependency::Info(type_idx, schedule_for_type.m_read_jobs.size(), Job_dependency::Info::Access_type::read));
				schedule_for_type.m_read_jobs.push_back(new_schedule_item);
			}
			else if constexpr (std::is_base_of<Processor_flag_write_base, T>::value)
			{
				Engine::Type_schedule::Item new_schedule_item;
				new_schedule_item.m_job = in_callback;
				new_schedule_item.m_id = in_id;

				Engine::Type_schedule& schedule_for_type = inout_engine.m_type_index_to_schedule[type_idx];
				schedule_for_type.m_typename = typeid(T).name();
				inout_dependency_infos.m_infos.push_back(Job_dependency::Info(type_idx, schedule_for_type.m_write_jobs.size(), Job_dependency::Info::Access_type::write));
				schedule_for_type.m_write_jobs.push_back(new_schedule_item);
			}
			else if constexpr (std::is_base_of<Processor_flag_access_single_base, T>::value)
			{
				Engine::Type_schedule::Item new_schedule_item;
				new_schedule_item.m_job = in_callback;
				new_schedule_item.m_id = in_id;

				Engine::Type_schedule& schedule_for_type = inout_engine.m_type_index_to_schedule[type_idx];
				schedule_for_type.m_typename = typeid(T).name();
				inout_dependency_infos.m_infos.push_back(Job_dependency::Info(type_idx, schedule_for_type.m_single_access_jobs.size(), Job_dependency::Info::Access_type::single_access));
				schedule_for_type.m_single_access_jobs.push_back(new_schedule_item);
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

		Engine* m_engine = nullptr;
	};

	template <typename ...T_processor_flags>
	struct Engine_processor : Processor_shared<T_processor_flags...>
	{
		friend struct Scene_context;
		friend Engine_context;

		explicit Engine_processor(Engine_context in_context) : Processor_shared(in_context.m_engine) {}
		explicit Engine_processor(Scene_context in_context) : Processor_shared(&in_context.m_engine) {}

		template<typename T_type>
		__forceinline void for_each(T_type in_func)
		{
			for_each_internal(std::function(in_func));
		}

		template<typename T_arg_type>
		__forceinline void for_each_internal(std::function<void(T_arg_type)> in_func)
		{
			static_assert(std::is_reference<T_arg_type>::value, "Type needs to be reference!");
			assert(m_engine);

			using Pure_type = std::remove_cv<std::remove_reference<T_arg_type>::template type>::template type;

			if constexpr (std::is_const<std::remove_reference<T_arg_type>::type>::value)
			{
				static_assert(verify_flags<Processor_flag_read<Pure_type>>() || verify_flags<Processor_flag_write<Pure_type>>(), "Processor does not have read flag for T_type");
				Engine_context(*m_engine).for_each_r(in_func);
			}
			else
			{
				static_assert(verify_flags<Processor_flag_write<Pure_type>>(), "Processor does not have write flag for T_type");
				Engine_context(*m_engine).for_each_w(in_func);
			}
		}

		template <typename T>
		Job_id schedule(T in_job, Schedule_data in_data = Schedule_data())
		{
			return Engine_context(*m_engine).schedule(std::function(in_job), in_data);
		}

		template <typename ...T_other_processor_flags>
		operator Engine_processor<T_other_processor_flags...>() const
		{
			static_assert
			(
				std::is_same<Engine_processor<>, Engine_processor<T_other_processor_flags...>>::value || (verify_flags<T_other_processor_flags>() && ...)
				, "Processor can only be converted if it has T_processor_type as part of its own <T...>"
			);

			return *((const Engine_processor<T_other_processor_flags...>*)((const void*)(this)));
		}

		bool get_does_object_exist(Object_id in_id) const { return Engine_context(*m_engine).get_does_object_exist(in_id); }
	};

	template <typename ...T_processor_flags>
	struct Scene_processor : Processor_shared<T_processor_flags...>
	{
		friend struct Scene_context;
		friend Engine_context;

		explicit Scene_processor(Scene_context in_context) : Processor_shared(&in_context.m_engine), m_scene(&in_context.m_scene) {}
		
		template<typename T_type>
		__forceinline void for_each(T_type in_func)
		{
			for_each_internal(std::function(in_func));
		}

		template<typename T_arg_type>
		__forceinline void for_each_internal(std::function<void(T_arg_type)> in_func)
		{
			static_assert(std::is_reference<T_arg_type>::value, "Type needs to be reference!");
			assert(m_engine);
			assert(m_scene);

			using Pure_type = std::remove_cv<std::remove_reference<T_arg_type>::template type>::template type;

			if constexpr (std::is_const<std::remove_reference<T_arg_type>::type>::value)
			{
				static_assert(verify_flags<Processor_flag_read<Pure_type>>() || verify_flags<Processor_flag_write<Pure_type>>(), "Processor does not have read flag for T_type");
				Scene_context(*m_engine, *m_scene).for_each_r(in_func);
			}
			else
			{
				static_assert(verify_flags<Processor_flag_write<Pure_type>>(), "Processor does not have write flag for T_type");
				Scene_context(*m_engine, *m_scene).for_each_w(in_func);
			}
		}

		template <typename T>
		Job_id schedule(T in_job, Schedule_data in_data = Schedule_data())
		{
			return Scene_context(*m_engine, *m_scene).schedule(std::function(in_job), in_data);
		}

		template <typename ...T_other_processor_flags>
		operator Scene_processor<T_other_processor_flags...>() const
		{
			static_assert
			(
				std::is_same<Scene_processor<>, Scene_processor<T_other_processor_flags...>>::value || (verify_flags<T_other_processor_flags>() && ...)
				, "Processor can only be converted if it has T_processor_type as part of its own <T...>"
			);

			return *((const Scene_processor<T_other_processor_flags...>*)((const void*)(this)));
		}

		Engine_processor<T_processor_flags...> get_engine_processor() const
		{
			return Engine_processor<T_processor_flags...>(Engine_context(*m_engine));
		}

		bool get_does_object_exist(Object_id in_id, bool in_only_in_this_scene) const { return Scene_context(*m_engine, *m_scene).get_does_object_exist(in_id, in_only_in_this_scene); }

		Scene* m_scene = nullptr;
	};

	struct Object_base;
	
	struct Scene_context
	{
		template <typename T_subtype, typename ...T_component>
		friend struct Object;

		template <typename ...T_processor_flags>
		friend struct Engine_processor;

		template <typename ...T_processor_flags>
		friend struct Scene_processor;

		friend Engine_context;

		friend struct Schedule_for_type;

		Scene_context(Engine& inout_engine, Scene& inout_scene) : m_engine(inout_engine), m_scene(inout_scene){}
		Scene_context(const Scene_context& in_context) : m_engine(in_context.m_engine), m_scene(in_context.m_scene) {}

		template <typename T_object>
		__forceinline constexpr T_object& create_object()
		{
			return m_scene.create_object<T_object>();
		}

		void destroy_object(Object_base& in_object_to_destroy)
		{
			m_scene.destroy_object(in_object_to_destroy);
		}

		void destroy_object_immediate(Object_base& inout_object_to_destroy)
		{
			m_scene.destroy_object_immediate(inout_object_to_destroy);
		}

		template<typename T_type>
		__forceinline void for_each_w(std::function<void(T_type&)> in_func)
		{
			m_scene.for_each_w<T_type>(in_func);
		}

		template<typename T_type>
		__forceinline void for_each_r(std::function<void(const T_type&)> in_func) const
		{
			m_scene.for_each_w<T_type>(in_func);
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

		template <typename T>
		__forceinline Job_id schedule(T in_job, Schedule_data in_data = Schedule_data())
		{
			return schedule_internal(std::function(in_job), in_data);
		}

		template <typename T>
		__forceinline Schedule_timed_handle schedule_timed(T in_job, Schedule_timed_info in_info)
		{
			return schedule_timed_internal(std::function(in_job), in_info);
		}

		template <typename ...T>
		__forceinline Job_id schedule_internal(std::function<void(Scene_processor<T...>)> in_job, Schedule_data in_data = Schedule_data())
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
					[in_job, &scene = m_scene](Engine_context in_context, Job_id in_job_id)
					{
						std::function<void()> job_callback =
							[in_job, context = Scene_context(*in_context.m_engine, scene)]()
						{
							Scene_processor<T...> processor(context);
							in_job(processor);
						};
						
						if constexpr (std::is_same<Scene_processor<T...>, Scene_processor<>>::value)
						{
							Engine::Type_schedule::Item new_schedule_item;
							new_schedule_item.m_job = job_callback;
							new_schedule_item.m_id = in_job_id;

							in_context.m_engine->m_no_flag_jobs.push_back(new_schedule_item);
						}
						else
						{
							const size_t job_index_offset = in_context.m_engine->m_thread_contexts[static_cast<size_t>(in_job_id.m_submitted_on_thread_id)]->m_job_index_offset;
							auto& dependency_infos = in_context.m_engine->m_job_id_to_type_dependencies_lut[job_index_offset + static_cast<size_t>(in_job_id.m_id)];
							(Scene_processor<T...>::schedule_for_type<T>(*in_context.m_engine, dependency_infos, job_callback, in_job_id), ...);
						}
					}
				}
			);

			return job_id;
		}

		template <typename ...T>
		__forceinline Schedule_timed_handle schedule_timed_internal(std::function<void(Scene_processor<T...>)> in_job, Schedule_timed_info in_info)
		{
			Schedule_timed_handle handle;

			auto& thread_this = this_thread();

			handle.m_thread_id = thread_this.get_id();
			handle.m_unique_id = thread_this.m_timed_schedule_handle_id_ticker++;

			Job_id job_id;
			job_id.m_id = -1;
			job_id.m_submitted_on_thread_id = thread_this.get_id();
			job_id.m_run_on_main_thread = in_info.m_run_on_main_thread;

			Thread_context::Timed_scheduled_item* new_item = nullptr;

			if (thread_this.m_free_timed_scheduled_items_indices.empty())
			{
				handle.m_job_index = thread_this.m_timed_scheduled_items.size();
				new_item = &thread_this.m_timed_scheduled_items.emplace_back();
			}
			else
			{
				handle.m_job_index = thread_this.m_free_timed_scheduled_items_indices.back();
				new_item = &thread_this.m_timed_scheduled_items[handle.m_job_index];
				thread_this.m_free_timed_scheduled_items_indices.pop_back();
			}

			*new_item =
			Thread_context::Timed_scheduled_item
			{
				[job_id, job_idx = handle.m_job_index, &thread_this, &scene = m_scene](Engine_context in_context) mutable
				{
					std::function<void()> job_callback = [job_idx, in_context, &thread_this, context = Scene_context(*in_context.m_engine, scene)]()
					{
						auto& job = *reinterpret_cast<std::function<void(Scene_processor<T...>)>*>(&thread_this.m_timed_scheduled_items[job_idx].m_job_func);
						Scene_processor<T...> processor(context);
						job(processor);
					};

					job_id.m_id = thread_this.m_job_id_ticker++;

					if constexpr (std::is_same<Scene_processor<T...>, Scene_processor<>>::value)
					{
						Engine::Type_schedule::Item new_schedule_item;
						new_schedule_item.m_job = job_callback;
						new_schedule_item.m_id = job_id;

						in_context.m_engine->m_no_flag_jobs.push_back(new_schedule_item);
					}
					else
					{
						auto& dependency_infos = in_context.m_engine->m_job_id_to_type_dependencies_lut[thread_this.m_job_index_offset + job_id.m_id];
						(Scene_processor<T...>::schedule_for_type<T>(*in_context.m_engine, dependency_infos, job_callback, job_id), ...);
					}
				},
				in_info.m_unschedule_callback,
				*reinterpret_cast<std::function<void()>*>(&in_job),
				in_info.m_duration,
				in_info.m_delay_until_start,
				0,
				handle.m_unique_id,
				m_scene.m_scene_id
			};

			return handle;
		}

		Engine_context get_engine_context() { return m_engine; }
		const Engine_context get_engine_context() const { return m_engine; }
		
		i32 get_outermost_scene_id() const { return m_scene.m_outermost_scene != nullptr ? m_scene.m_outermost_scene->m_scene_id : get_scene_id(); }
		i32 get_scene_id() const { return m_scene.m_scene_id; }
		bool get_does_object_exist(Object_id in_id, bool in_only_in_this_scene) const { return m_scene.get_does_object_exist(in_id, in_only_in_this_scene); }

	private:
		Engine& m_engine;
		Scene& m_scene;
	};

	template <typename ...T_processor_flags>
	using Ep = Engine_processor<T_processor_flags...>;
	template <typename ...T_processor_flags>
	using Sp = Scene_processor<T_processor_flags...>;
}


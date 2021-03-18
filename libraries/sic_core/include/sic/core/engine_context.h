#pragma once
#include "sic/core/engine.h"
#include "sic/core/scene.h"
#include "sic/core/type.h"

namespace sic
{
	struct Scene_context;

	struct Schedule_data
	{
		Schedule_data& job_dependency(Job_id in_job_id) { m_job_dependency = in_job_id; return *this; }
		Schedule_data& run_on_main_thread(bool in_run_on_main_thread) { m_run_on_main_thread = in_run_on_main_thread; return *this; }

		std::optional<Job_id> m_job_dependency;
		bool m_run_on_main_thread = false;
	};

	struct Schedule_timed_info
	{
		friend struct Scene_context;
		friend struct Engine_context;

		Schedule_timed_info& unschedule_callback(std::function<bool(Engine_context)> in_callback) { m_unschedule_callback = in_callback; return *this; }
		Schedule_timed_info& duration(float in_duration) { m_duration = in_duration; return *this; }
		Schedule_timed_info& delay_until_start(float in_delay_until_start) { m_delay_until_start = in_delay_until_start; return *this; }
		Schedule_timed_info& run_on_main_thread(bool in_run_on_main_thread) { m_run_on_main_thread = in_run_on_main_thread; return *this; }

	private:
		std::function<bool(Engine_context)> m_unschedule_callback;
		float m_duration = 0.0f;
		float m_delay_until_start = 0.0f;
		bool m_run_on_main_thread = false;
	};

	struct Engine_context
	{
		template <typename ...T_processor_flags>
		friend struct Engine_processor;

		friend struct Scene_context;

		Engine_context() = default;
		Engine_context(Engine& in_engine) : m_engine(&in_engine) {}

		template<typename T_system_type>
		__forceinline T_system_type& create_subsystem(System& inout_system)
		{
			auto& new_system = m_engine->create_system<T_system_type>();
			inout_system.m_subsystems.push_back(&new_system);

			return new_system;
		}

		template <typename T_component_type>
		__forceinline constexpr void register_component(const char* in_unique_key, ui32 in_initial_capacity = 128)
		{
			m_engine->m_registration_callbacks.push_back
			(
				[in_initial_capacity](Scene& inout_scene)
				{
					const ui32 type_idx = Type_index<Component_base>::get<T_component_type>();

					while (type_idx >= inout_scene.m_component_storages.size())
						inout_scene.m_component_storages.push_back(nullptr);

					Component_storage<T_component_type>* new_storage = new Component_storage<T_component_type>();
					new_storage->initialize(in_initial_capacity);
					inout_scene.m_component_storages[type_idx] = std::unique_ptr<Component_storage_base>(new_storage);
				}
			);

			register_typeinfo<T_component_type>(in_unique_key);
		}

		template <typename T_object>
		__forceinline constexpr void register_object(const char* in_unique_key, ui32 in_initial_capacity = 128, ui32 in_bucket_capacity = 64)
		{
			static_assert(std::is_base_of<Object_base, T_object>::value, "object must derive from struct Object<>");

			m_engine->m_registration_callbacks.push_back
			(
				[in_initial_capacity, in_bucket_capacity](Scene& inout_scene)
				{
					const ui32 type_idx = Type_index<Object_base>::get<T_object>();

					while (type_idx >= inout_scene.m_objects.size())
						inout_scene.m_objects.push_back(nullptr);

					auto& new_object_storage = inout_scene.get_object_storage_at_index(type_idx);

					assert(new_object_storage.get() == nullptr && "object is already registered");

					new_object_storage = std::make_unique<Object_storage>();
					reinterpret_cast<Object_storage*>(new_object_storage.get())->initialize_with_typesize(in_initial_capacity, in_bucket_capacity, sizeof(T_object));

				}
			);

			register_typeinfo<T_object>(in_unique_key);
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
			m_engine->m_rtti->register_typeinfo<T_type_to_register>(in_unique_key);
		}

		template <typename T_type>
		__forceinline constexpr const rtti::Typeinfo* find_typeinfo() const
		{
			return m_engine->m_rtti->find_typeinfo<T_type>();
		}

		const rtti::Rtti& get_rtti() const
		{
			return *m_engine->m_rtti.get();
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
		__forceinline Job_id schedule_internal(std::function<void(Engine_processor<T...>)> in_job, Schedule_data in_data = Schedule_data())
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
						std::function<void()> job_callback =
							[in_job, in_context]()
						{
							Engine_processor<T...> processor(in_context);
							in_job(processor);
						};

						if constexpr (std::is_same<Engine_processor<T...>, Engine_processor<>>::value)
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
							(Engine_processor<T...>::schedule_for_type<T>(*in_context.m_engine, dependency_infos, job_callback, in_job_id), ...);
						}
					}
				}
			);

			return job_id;
		}

		template <typename ...T>
		__forceinline Schedule_timed_handle schedule_timed_internal(std::function<void(Engine_processor<T...>)> in_job, Schedule_timed_info in_info)
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
				[job_id, job_idx = handle.m_job_index, &thread_this](Engine_context in_context) mutable
				{
					std::function<void()> job_callback = [job_idx, in_context, &thread_this]()
					{
						auto& job = *reinterpret_cast<std::function<void(Engine_processor<T...>)>*>(&thread_this.m_timed_scheduled_items[job_idx].m_job_func);
						Engine_processor<T...> processor(in_context);
						job(processor);
					};

					job_id.m_id = thread_this.m_job_id_ticker++;

					if constexpr (std::is_same<Engine_processor<T...>, Engine_processor<>>::value)
					{
						Engine::Type_schedule::Item new_schedule_item;
						new_schedule_item.m_job = job_callback;
						new_schedule_item.m_id = job_id;

						in_context.m_engine->m_no_flag_jobs.push_back(new_schedule_item);
					}
					else
					{
						auto& dependency_infos = in_context.m_engine->m_job_id_to_type_dependencies_lut[thread_this.m_job_index_offset + job_id.m_id];
						(Engine_processor<T...>::schedule_for_type<T>(*in_context.m_engine, dependency_infos, job_callback, job_id), ...);
					}
				},
				in_info.m_unschedule_callback,
				*reinterpret_cast<std::function<void()>*>(&in_job),
				in_info.m_duration,
				in_info.m_delay_until_start,
				0,
				handle.m_unique_id,
				{}
			};

			return handle;
		}

		template <typename T_custom_schedule_type>
		void schedule_timed_custom(T_custom_schedule_type in_schedule_data)
		{
			schedule_timed
			(
				in_schedule_data.get_callback(),
				Schedule_timed_info()
				.duration(in_schedule_data.get_duration())
				.delay_until_start(in_schedule_data.get_delay_until_start())
				.run_on_main_thread(in_schedule_data.get_run_on_main_thread())
				.unschedule_callback(in_schedule_data.get_unschedule_callback())
			);
		}

		void unschedule(Schedule_timed_handle in_handle)
		{
			if (in_handle.get_thread_id() == -1)
				return;

			this_thread().m_scheduled_items_to_remove.emplace_back(in_handle);
		}

		void create_scene(Scene* in_parent_scene, std::function<void(Scene_context)> in_on_created_callback = {})
		{
			m_engine->create_scene(in_parent_scene, in_on_created_callback);
		}

		void destroy_scene(Scene_context in_context);

		std::optional<Scene_context> find_scene(sic::i32 in_scene_index);

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

		void set_profiling(bool in_should_profile)
		{
			m_engine->m_is_profiling = in_should_profile;
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


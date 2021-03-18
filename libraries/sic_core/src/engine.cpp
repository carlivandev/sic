#include "sic/core/engine.h"
#include "sic/core/object.h"
#include "sic/core/engine_context.h"
#include "sic/core/engine_job_scheduling.h"

#include <algorithm>
#include <atomic>

namespace sic
{
	void Engine::finalize()
	{
		assert(!m_initialized && "");

		m_initialized = true;
		m_is_shutting_down = false;

		refresh_time_delta();

		prepare_threadpool();

		m_finished_setup = true;

		for (auto&& system : m_systems)
			system->execute_engine_finalized(Engine_context(*this));

		while (!m_type_index_to_schedule.empty())
			execute_scheduled_jobs();
	}

	void Engine::simulate()
	{
		refresh_time_delta();

		if (is_shutting_down())
			return;

		if (!m_initialized)
			return;

		if (!is_shutting_down())
			tick();

		if (is_shutting_down())
			on_shutdown();
	}

	void Engine::prepare_threadpool()
	{
		const auto hardware_thread_count = std::thread::hardware_concurrency() - 2;

		m_thread_contexts.resize(hardware_thread_count);

		m_system_ticker_threadpool.spawn
		(
			static_cast<ui16>(hardware_thread_count) - 1,
			[this](i32 in_index)
			{
				m_thread_contexts[in_index + 1] = &this_thread();
				this_thread().m_id = in_index + 1;
				this_thread().set_name("worker_thread: " + std::to_string(in_index + 1));
			}
		);

		m_thread_contexts[0] = &this_thread();
		this_thread().m_id = 0;
		this_thread().set_name("main thread");
	}

	void Engine::tick()
	{
		tick_systems(m_pre_tick_systems);
		execute_scheduled_jobs();

		flush_deferred_updates();

		tick_systems(m_tick_systems);
		execute_scheduled_jobs();

		flush_deferred_updates();

		tick_systems(m_post_tick_systems);
		execute_scheduled_jobs();

		flush_deferred_updates();

		execute_scheduled_jobs();

		flush_scene_streaming();

		for (Thread_context* context : m_thread_contexts)
			context->reset_local_storage();
	}

	void Engine::on_shutdown()
	{
		m_system_ticker_threadpool.shutdown();

		while (!m_scenes.empty())
			destroy_scene(*m_scenes.back().get());

		for (auto& system : m_systems)
			system->on_shutdown(Engine_context(*this));
	}

	void Engine::create_scene(Scene* in_parent_scene)
	{
		std::scoped_lock scenes_lock(m_scenes_mutex);

		assert(m_finished_setup && "Can't create a scene before system has finished setup!");

		Scene* outermost_scene = nullptr;

		if (in_parent_scene)
		{
			if (in_parent_scene->m_outermost_scene)
				outermost_scene = in_parent_scene->m_outermost_scene;
			else
				outermost_scene = in_parent_scene;
		}

		m_scenes_to_add.push_back(std::make_unique<Scene>(*this, outermost_scene, in_parent_scene));

		Scene& new_scene = *m_scenes_to_add.back().get();
		new_scene.m_scene_id = m_scene_id_ticker++;

		for (auto&& registration_callback : m_registration_callbacks)
			registration_callback(new_scene);
	}

	void Engine::destroy_scene(Scene& inout_scene)
	{
		std::scoped_lock scenes_lock(m_scenes_mutex);

		for (size_t i = 0; i < m_scenes.size(); i++)
		{
			if (m_scenes[i].get() != &inout_scene)
				continue;

			for (auto& system : m_systems)
			{
				if (!is_shutting_down())
					system->on_end_simulation(Scene_context(*this, inout_scene));
			}

			m_scenes.erase(m_scenes.begin() + i);
			break;
		}
	}

	void Engine::refresh_time_delta()
	{
		const auto now = std::chrono::high_resolution_clock::now();
		const auto dif = now - m_previous_frame_time_point;
		m_time_delta = std::chrono::duration<float, std::milli>(dif).count() * 0.001f;
		m_previous_frame_time_point = now;

		++m_current_frame;
	}

	void Engine::flush_scene_streaming()
	{
		if (!m_scenes_to_remove.empty())
		{
			std::scoped_lock scenes_lock(m_scenes_mutex);

			//first remove scenes
			for (Scene* scene : m_scenes_to_remove)
				destroy_scene_internal(*scene);

			m_scenes_to_remove.clear();
		}

		if (!m_scenes_to_add.empty())
		{
			std::vector<Scene*> added_scenes;

			{
				std::scoped_lock scenes_lock(m_scenes_mutex);
				//then add new scenes
				for (auto& scene : m_scenes_to_add)
				{
					Scene* added_scene = nullptr;
					//is this a subscene?
					if (scene->m_outermost_scene)
					{
						scene->m_outermost_scene->m_subscenes.push_back(std::move(scene));
						added_scene = scene->m_outermost_scene->m_subscenes.back().get();
					}
					else
					{
						m_scenes.push_back(std::move(scene));
						added_scene = m_scenes.back().get();
						m_scene_id_to_scene_lut[added_scene->m_scene_id] = added_scene;
					}

					added_scenes.push_back(added_scene);
				}

				m_scenes_to_add.clear();
			}

			for (Scene* added_scene : added_scenes)
			{
				invoke<Event_created<Scene>>(added_scene);

				for (auto& system : m_systems)
				{
					if (!is_shutting_down())
						system->on_begin_simulation(Scene_context(*this, *added_scene));
				}
			}
		}
	}

	void Engine::tick_systems(std::vector<System*>& inout_systems)
	{
		for (auto& tick_system : inout_systems)
		{
			for (auto& scene : m_scenes)
				tick_system->execute_tick(Scene_context(*this, *scene.get()), m_time_delta);

			tick_system->execute_engine_tick(Engine_context(*this), m_time_delta);
		}

	}

	void Engine::execute_scheduled_jobs()
	{
		size_t job_count = 0;

		for (auto&& thread : m_thread_contexts)
		{
			thread->m_job_index_offset = job_count;
			job_count += thread->m_scheduled_items.size();

			for (auto& scheduled_item : thread->m_scheduled_items)
				scheduled_item.second(Engine_context(*this), scheduled_item.first);

			thread->m_scheduled_items.clear();

			for (size_t schedule_item_idx = 0; schedule_item_idx < thread->m_timed_scheduled_items.size(); ++schedule_item_idx)
			{
				auto& scheduled_item = thread->m_timed_scheduled_items[schedule_item_idx];

				if (!scheduled_item.m_func)
					continue;

				if (scheduled_item.m_frame_when_ticked == m_current_frame)
					continue;

				scheduled_item.m_frame_when_ticked = m_current_frame;

				if (scheduled_item.m_time_until_start_scheduling <= 0.0f)
				{
					++job_count;
					scheduled_item.m_func(Engine_context(*this));

					scheduled_item.m_time_left -= m_time_delta;
					if (scheduled_item.m_time_left <= 0.0f)
					{
						scheduled_item.m_func = nullptr;
						thread->m_free_timed_scheduled_items_indices.push_back(schedule_item_idx);
						continue;
					}
				}
				else
				{
					scheduled_item.m_time_until_start_scheduling -= m_time_delta;
				}
			}
		}

		Main_thread_worker main_thread_worker;
		std::vector<Job_node> job_nodes(job_count);
		std::vector<Job_node*> leaf_nodes;

		auto schedule_jobs =
		[
			this,
			&job_nodes,
			&main_thread_worker
		]
		(const std::vector<Type_schedule::Item>& in_jobs)
		{
			for (auto&& current_job : in_jobs)
			{
				const size_t thread_job_index_offset = m_thread_contexts[current_job.m_id.m_submitted_on_thread_id]->m_job_index_offset;
				Job_node& current_node = job_nodes[thread_job_index_offset + current_job.m_id.m_id];

				//job already initialized
				if (current_node.m_job)
				{
					assert
					(
						current_job.m_id.m_submitted_on_thread_id == current_node.m_id.m_submitted_on_thread_id &&
						current_job.m_id.m_id == current_node.m_id.m_id
					);
					continue;
				}

				current_node.m_threadpool = &m_system_ticker_threadpool;
				current_node.m_main_thread_worker = &main_thread_worker;
				current_node.m_is_ready_to_execute = true;
				current_node.m_run_on_main_thread = current_job.m_id.m_run_on_main_thread;
				current_node.m_job = current_job.m_job;
				current_node.m_id = current_job.m_id;
				current_node.m_dependencies_left = 0;

				if (current_job.m_id.m_job_dependency.has_value())
				{
					++current_node.m_dependencies_left;

					Job_node& depends_on_node = job_nodes[m_thread_contexts[current_job.m_id.m_job_dependency.value().m_submitted_on_thread_id]->m_job_index_offset + current_job.m_id.m_job_dependency.value().m_id];
					depends_on_node.m_depends_on_me.push_back(&current_node);
				}

				//find the other type schedules where this job is referenced
				//grab the previous job_id in each of those lists, and add this node to their depends_on_me list

				auto& type_dependencies = m_job_id_to_type_dependencies_lut[thread_job_index_offset + current_job.m_id.m_id];
				for (auto& type_dependency_info : type_dependencies.m_infos)
				{
					if (type_dependency_info.m_index > 0)
					{
						if (type_dependency_info.m_access_type == Job_dependency::Info::Access_type::read)
						{
							//this is a read dependency. doesnt have any dependencies
							continue;
						}
						else if (type_dependency_info.m_access_type == Job_dependency::Info::Access_type::single_access)
						{
							//add all writes as dependencies, except if explicit dependency on read for current_node has been set

							for (auto&& write_dependency : m_type_index_to_schedule.find(type_dependency_info.m_type_index)->second.m_write_jobs)
							{
								const i32 thread_id_offset = m_thread_contexts[write_dependency.m_id.m_submitted_on_thread_id]->m_job_index_offset;
								const i32 id = write_dependency.m_id.m_id;
								Job_node& write_dependency_node = job_nodes[thread_id_offset + id];

								bool already_added = false;

								for (Job_node* prev_node_depends_on_me : write_dependency_node.m_depends_on_me)
								{
									if (prev_node_depends_on_me == &current_node)
									{
										already_added = true;
										break;
									}
								}

								//skip if explicit dependency on write has been set
								for (Job_node* current_node_depends_on_me : current_node.m_depends_on_me)
								{
									if (current_node_depends_on_me == &write_dependency_node)
									{
										already_added = true;
										break;
									}
								}

								if (!already_added)
								{
									write_dependency_node.m_depends_on_me.push_back(&current_node);
									++current_node.m_dependencies_left;
								}
							}

							continue;
						}
						else if (type_dependency_info.m_access_type == Job_dependency::Info::Access_type::write)
						{
							//add previous write as dependency

							auto& write_jobs = m_type_index_to_schedule.find(type_dependency_info.m_type_index)->second.m_write_jobs;
							if (write_jobs.size() <= type_dependency_info.m_index)
								continue;

							const i32 id = write_jobs[type_dependency_info.m_index - 1].m_id.m_id;
							const i32 thread_id_offset = m_thread_contexts[write_jobs[type_dependency_info.m_index - 1].m_id.m_submitted_on_thread_id]->m_job_index_offset;

							Job_node& previous_node_write = job_nodes[thread_id_offset + id];

							bool already_added = false;

							for (Job_node* prev_node_depends_on_me : previous_node_write.m_depends_on_me)
							{
								if (prev_node_depends_on_me == &current_node)
								{
									already_added = true;
									break;
								}
							}

							//skip if explicit dependency on write has been set
							for (Job_node* current_node_depends_on_me : current_node.m_depends_on_me)
							{
								if (current_node_depends_on_me == &previous_node_write)
								{
									already_added = true;
									break;
								}
							}

							if (!already_added)
							{
								previous_node_write.m_depends_on_me.push_back(&current_node);
								++current_node.m_dependencies_left;
							}
						}

						{
							//add add all reads as dependencies, except if explicit dependency on read for current_node has been set

							for (auto&& read_dependency : m_type_index_to_schedule.find(type_dependency_info.m_type_index)->second.m_read_jobs)
							{
								const i32 id = read_dependency.m_id.m_id;
								const i32 thread_id_offset = m_thread_contexts[read_dependency.m_id.m_submitted_on_thread_id]->m_job_index_offset;

								Job_node& read_dependency_node = job_nodes[thread_id_offset + id];

								bool already_added = false;

								for (Job_node* prev_node_depends_on_me : read_dependency_node.m_depends_on_me)
								{
									if (prev_node_depends_on_me == &current_node)
									{
										already_added = true;
										break;
									}
								}

								//skip if explicit dependency on read has been set
								for (Job_node* current_node_depends_on_me : current_node.m_depends_on_me)
								{
									if (current_node_depends_on_me == &read_dependency_node)
									{
										already_added = true;
										break;
									}
								}

								if (!already_added)
								{
									read_dependency_node.m_depends_on_me.push_back(&current_node);
									++current_node.m_dependencies_left;
								}
							}
						}
					}
				}
			}
		};

		for (auto& type_schedule : m_type_index_to_schedule)
		{
			schedule_jobs(type_schedule.second.m_read_jobs);
			schedule_jobs(type_schedule.second.m_write_jobs);
			schedule_jobs(type_schedule.second.m_deferred_write_jobs);
			schedule_jobs(type_schedule.second.m_single_access_jobs);
		}

		for (Job_node& job_node : job_nodes)
			assert(job_node.m_job);

		m_type_index_to_schedule.clear();
		m_job_id_to_type_dependencies_lut.clear();


		for (auto&& thread : m_thread_contexts)
		{
			thread->m_job_index_offset = 0;
			thread->m_job_id_ticker = 0;
		}

		std::vector<Threadpool::Closure> tasks_to_run_on_workers;

		for (Job_node& job_node : job_nodes)
		{
			if (job_node.m_dependencies_left == 0)
			{
				job_node.m_is_ready_to_execute = true;

				if (job_node.m_run_on_main_thread)
					main_thread_worker.push_back_job(&job_node);
				else
					tasks_to_run_on_workers.push_back([&job_node]() {job_node.do_job(); });
			}
			else
			{
				job_node.m_is_ready_to_execute = false;
			}
		}

		for (Job_node& node : job_nodes)
		{
			if (node.m_depends_on_me.empty())
				leaf_nodes.push_back(&node);
		}

		if (!tasks_to_run_on_workers.empty())
			m_system_ticker_threadpool.batch(std::move(tasks_to_run_on_workers));

		bool all_leaf_nodes_finished = false;

		while (!all_leaf_nodes_finished)
		{
			std::this_thread::yield();

			main_thread_worker.try_do_job();

			all_leaf_nodes_finished = true;

			for (const Job_node* leaf_node : leaf_nodes)
			{
				if (!leaf_node->m_job_finished)
				{
					all_leaf_nodes_finished = false;
					break;
				}
			}

			if (all_leaf_nodes_finished)
				break;
		}
	}

	void Engine::flush_deferred_updates()
	{
		for (Thread_context* context : m_thread_contexts)
		{
			for (size_t update_idx = 0; update_idx < context->m_deferred_updates.size(); ++update_idx)
			{
				const auto cur_update = context->m_deferred_updates[update_idx];
				cur_update(Engine_context(*this));
			}

			context->m_deferred_updates.clear();

			if (context != &sic::this_thread())
			{
				for (size_t update_idx = 0; update_idx < sic::this_thread().m_deferred_updates.size(); ++update_idx)
				{
					const auto cur_update = sic::this_thread().m_deferred_updates[update_idx];
					cur_update(Engine_context(*this));
				}

				sic::this_thread().m_deferred_updates.clear();
			}
		}
	}

	std::unique_ptr<State>& Engine::get_state_at_index(i32 in_index)
	{
		while (in_index >= m_states.size())
			m_states.push_back(nullptr);

		return m_states[in_index];
	}

	void Engine::destroy_scene_internal(Scene& in_scene)
	{
		//first, recursively destroy all subscenes
		const i32 last_subscene_index = static_cast<i32>(in_scene.m_subscenes.size()) - 1;

		for (i32 i = last_subscene_index; i >= 0; i--)
			destroy_scene_internal(*in_scene.m_subscenes[i].get());

		//then destroy in_scene

		for (auto& system : m_systems)
		{
			if (!is_shutting_down())
				system->on_end_simulation(Scene_context(*this, in_scene));
		}

		invoke<event_destroyed<Scene>>(&in_scene);

		m_scene_id_to_scene_lut.erase(in_scene.m_scene_id);

		std::vector<std::unique_ptr<Scene>>* scenes_to_remove_from;
		if (in_scene.m_parent_scene)
			scenes_to_remove_from = &in_scene.m_parent_scene->m_subscenes;
		else
			scenes_to_remove_from = &m_scenes;

		auto it = std::find_if(scenes_to_remove_from->begin(), scenes_to_remove_from->end(), [&in_scene](auto& other_scene) {return &in_scene == other_scene.get(); });
		if (it != scenes_to_remove_from->end())
			scenes_to_remove_from->erase(it);
	}
}
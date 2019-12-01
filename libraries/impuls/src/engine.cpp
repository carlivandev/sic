#include "impuls/pch.h"
#include "impuls/engine.h"
#include "impuls/object.h"
#include "impuls/engine_context.h"

namespace impuls
{
	void engine::initialize()
	{
		assert(!m_initialized && "");

		m_initialized = true;
		m_is_destroyed = false;

		refresh_time_delta();
	}

	void engine::simulate()
	{
		refresh_time_delta();

		if (is_destroyed())
			return;

		if (!m_initialized)
			return;

		if (!m_begun_simulation)
		{
			begin_simulation();
			m_begun_simulation = true;
		}

		if (m_begun_simulation)
		{
			if (!is_destroyed())
				tick();

			if (is_destroyed())
				end_simulation();
		}
	}

	void engine::begin_simulation()
	{
		for (auto& system : m_systems)
		{
			if (!m_is_destroyed)
				system->on_begin_simulation(std::move(engine_context(*this, *system.get())));
		}

		size_t most_parallel_possible = 0;
		for (auto& system : m_systems)
		{
			if (system->m_subsystems.size() > most_parallel_possible)
				most_parallel_possible = system->m_subsystems.size();
		}

		most_parallel_possible += m_async_ticksteps.size();

		m_tickstep_threadpool.spawn(static_cast<ui16>(most_parallel_possible));

		for (auto& async_tickstep : m_async_ticksteps)
		{
			m_tickstep_threadpool.emplace
			(
				[this, async_tickstep]()
				{
					while (!m_tickstep_threadpool.is_shutting_down())
					{
						//todo: add time delta support on async systems
						async_tickstep->execute_tick(engine_context(*this, *async_tickstep), 0.0f);
					}
				}
			);
		}

		m_begun_simulation = true;
	}

	void engine::tick()
	{
		std::vector<threadpool::closure> tasks_to_run;
		tasks_to_run.reserve(8);

		for (auto& synced_tickstep : m_synced_ticksteps)
		{
			tasks_to_run.clear();

			if (synced_tickstep.size())
			{
				for (ui32 system_idx = 1; system_idx < synced_tickstep.size(); system_idx++)
				{
					i_system* system_to_tick = synced_tickstep[system_idx];

					tasks_to_run.emplace_back
					(
						[this, system_to_tick]()
						{
							system_to_tick->execute_tick(engine_context(*this, *system_to_tick), m_time_delta);
							system_to_tick->m_finished_tick = true;
						}
					);

					system_to_tick->m_finished_tick = false;
				}

				synced_tickstep[0]->execute_tick(engine_context(*this, *synced_tickstep[0]), m_time_delta);
				synced_tickstep[0]->m_finished_tick = true;
			}

			if (!tasks_to_run.empty())
				m_tickstep_threadpool.batch(std::move(tasks_to_run));

			//wait for all to finish
			bool all_finished = false;

			while (!all_finished)
			{
				all_finished = true;

				for (i_system* system_to_tick : synced_tickstep)
				{
					if (!system_to_tick->m_finished_tick)
						all_finished = false;
				}
			}

		}

		/*
		for (auto& system : m_systems)
		{
			if (!m_is_destroyed)
				system->execute_tick(std::move(engine_context(*this, *system.get())), m_time_delta);
		}
		*/
	}

	void engine::end_simulation()
	{
		m_tickstep_threadpool.shutdown();

		for (auto& system : m_systems)
		{
			if (!m_is_destroyed)
				system->on_end_simulation(std::move(engine_context(*this, *system.get())));
		}
	}

	void engine::refresh_time_delta()
	{
		const auto now = std::chrono::high_resolution_clock::now();
		const auto dif = now - m_previous_frame_time_point;
		m_time_delta = std::chrono::duration<float, std::milli>(dif).count() * 0.001f;
		m_previous_frame_time_point = now;
	}

	std::unique_ptr<i_component_storage>& engine::get_component_storage_at_index(i32 in_index)
	{
		return m_component_storages[in_index];
	}

	std::unique_ptr<i_object_storage_base>& engine::get_object_storage_at_index(i32 in_index)
	{
		while (in_index >= m_objects.size())
			m_objects.push_back(nullptr);

		return m_objects[in_index];
	}

	std::unique_ptr<i_state>& engine::get_state_at_index(i32 in_index)
	{
		while (in_index >= m_states.size())
			m_states.push_back(nullptr);

		return m_states[in_index];
	}
}
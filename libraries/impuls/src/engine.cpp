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
		m_is_shutting_down = false;

		refresh_time_delta();
	}

	void engine::simulate()
	{
		refresh_time_delta();

		if (is_shutting_down())
			return;

		if (!m_initialized)
			return;

		if (!m_has_setup_async_ticksteps)
		{
			setup_async_ticksteps();
			m_has_setup_async_ticksteps = true;
		}

		if (!m_finished_setup)
		{
			m_finished_setup = true;

			//first level always reserved for engine
			create_level();
			flush_level_streaming();
		}

		if (!is_shutting_down())
			tick();

		if (is_shutting_down())
			on_shutdown();
	}

	void engine::setup_async_ticksteps()
	{
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
					while (!m_finished_setup)
					{
						std::this_thread::sleep_for(std::chrono::milliseconds(1));
						continue;
					}

					while (!m_tickstep_threadpool.is_shutting_down())
					{
						//cant change levels list while in the middle of a tick
						std::scoped_lock levels_lock(m_levels_mutex);

						//todo: add time delta support on async systems

						for (auto& level : m_levels)
							async_tickstep->execute_tick(level_context(*this, *level.get()), 0.0f);

						async_tickstep->execute_engine_tick(engine_context(*this), 0.0f);
					}
				}
			);
		}
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
							for (auto& level : m_levels)
								system_to_tick->execute_tick(level_context(*this, *level.get()), m_time_delta);

							system_to_tick->execute_engine_tick(engine_context(*this), m_time_delta);
							system_to_tick->m_finished_tick = true;
						}
					);

					system_to_tick->m_finished_tick = false;
				}

				for (auto& level : m_levels)
					synced_tickstep[0]->execute_tick(level_context(*this, *level.get()), m_time_delta);

				synced_tickstep[0]->execute_engine_tick(engine_context(*this), m_time_delta);
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

		flush_level_streaming();
	}

	void engine::on_shutdown()
	{
		m_tickstep_threadpool.shutdown();

		while (!m_levels.empty())
			destroy_level(*m_levels.back().get());
	}

	void engine::create_level()
	{
		std::scoped_lock levels_lock(m_levels_mutex);

		assert(m_finished_setup && "Can't create a level before system has finished setup!");

		m_levels_to_add.push_back(std::make_unique<level>(*this));

		level& new_level = *m_levels_to_add.back().get();

		for (auto&& registration_callback : m_registration_callbacks)
			registration_callback(new_level);
	}

	void engine::destroy_level(level& inout_level)
	{
		std::scoped_lock levels_lock(m_levels_mutex);

		for (size_t i = 0; i < m_levels.size(); i++)
		{
			if (m_levels[i].get() != &inout_level)
				continue;

			for (auto& system : m_systems)
			{
				if (!is_shutting_down())
					system->on_end_simulation(level_context(*this, inout_level));
			}

			m_levels.erase(m_levels.begin() + i);
			break;
		}
	}

	void engine::refresh_time_delta()
	{
		const auto now = std::chrono::high_resolution_clock::now();
		const auto dif = now - m_previous_frame_time_point;
		m_time_delta = std::chrono::duration<float, std::milli>(dif).count() * 0.001f;
		m_previous_frame_time_point = now;
	}

	void engine::flush_level_streaming()
	{
		//first remove levels
		for (level* level : m_levels_to_remove)
		{
			for (auto& system : m_systems)
			{
				if (!is_shutting_down())
					system->on_end_simulation(level_context(*this, *level));
			}

			auto it = std::find_if(m_levels.begin(), m_levels.end(), [level](auto& other_level) {return level == other_level.get(); });
			if (it != m_levels.end())
				m_levels.erase(it);
		}

		m_levels_to_remove.clear();

		//then add new levels
		for (auto& level : m_levels_to_add)
		{
			m_levels.push_back(std::move(level));

			for (auto& system : m_systems)
			{
				if (!is_shutting_down())
					system->on_begin_simulation(level_context(*this, *m_levels.back().get()));
			}
		}

		m_levels_to_add.clear();
	}

	std::unique_ptr<i_state>& engine::get_state_at_index(i32 in_index)
	{
		while (in_index >= m_states.size())
			m_states.push_back(nullptr);

		return m_states[in_index];
	}
}
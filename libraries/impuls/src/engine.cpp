#include "impuls/pch.h"
#include "impuls/engine.h"
#include "impuls/object.h"
#include "impuls/engine_context.h"

namespace impuls
{
	void Engine::initialize()
	{
		assert(!m_initialized && "");

		m_initialized = true;
		m_is_shutting_down = false;

		refresh_time_delta();
	}

	void Engine::simulate()
	{
		refresh_time_delta();

		if (is_shutting_down())
			return;

		if (!m_initialized)
			return;

		if (!m_has_prepared_threadpool)
		{
			prepare_threadpool();
			m_has_prepared_threadpool = true;
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

	void Engine::prepare_threadpool()
	{
		const size_t most_parallel_possible = m_async_systems.size() + m_tick_systems.size() - 1;

		m_system_ticker_threadpool.spawn(static_cast<ui16>(most_parallel_possible));

		for (auto& async_system : m_async_systems)
		{
			m_system_ticker_threadpool.emplace
			(
				[this, async_system]()
				{
					while (!m_finished_setup)
					{
						std::this_thread::sleep_for(std::chrono::milliseconds(1));
						continue;
					}

					while (!m_system_ticker_threadpool.is_shutting_down())
					{
						//cant change levels list while in the middle of a tick
						std::scoped_lock levels_lock(m_levels_mutex);

						//todo: add time delta support on async systems

						for (auto& level : m_levels)
							async_system->execute_tick(Level_context(*this, *level.get()), 0.0f);

						async_system->execute_engine_tick(Engine_context(*this), 0.0f);
					}
				}
			);
		}
	}

	void Engine::tick()
	{
		for (auto& pre_tick_system : m_pre_tick_systems)
		{
			for (auto& level : m_levels)
				pre_tick_system->execute_tick(Level_context(*this, *level.get()), m_time_delta);

			pre_tick_system->execute_engine_tick(Engine_context(*this), m_time_delta);
		}

		if (!m_tick_systems.empty())
		{
			std::vector<Threadpool::closure> tasks_to_run;
			tasks_to_run.reserve(m_tick_systems.size() - 1);

			for (ui32 system_idx = 1; system_idx < m_tick_systems.size(); system_idx++)
			{
				System* system_to_tick = m_tick_systems[system_idx];

				tasks_to_run.emplace_back
				(
					[this, system_to_tick]()
					{
						for (auto& level : m_levels)
							system_to_tick->execute_tick(Level_context(*this, *level.get()), m_time_delta);

						system_to_tick->execute_engine_tick(Engine_context(*this), m_time_delta);
						system_to_tick->m_finished_tick = true;
					}
				);

				system_to_tick->m_finished_tick = false;
			}

			if (!tasks_to_run.empty())
				m_system_ticker_threadpool.batch(std::move(tasks_to_run));

			for (auto& level : m_levels)
				m_tick_systems[0]->execute_tick(Level_context(*this, *level.get()), m_time_delta);

			m_tick_systems[0]->execute_engine_tick(Engine_context(*this), m_time_delta);
			m_tick_systems[0]->m_finished_tick = true;
		}

		//wait for all to finish
		bool all_finished = false;

		while (!all_finished)
		{
			all_finished = true;

			for (System* system_to_tick : m_tick_systems)
			{
				if (!system_to_tick->m_finished_tick)
					all_finished = false;
			}
		}

		for (auto& post_tick_system : m_post_tick_systems)
		{
			for (auto& level : m_levels)
				post_tick_system->execute_tick(Level_context(*this, *level.get()), m_time_delta);

			post_tick_system->execute_engine_tick(Engine_context(*this), m_time_delta);
		}

		flush_level_streaming();
	}

	void Engine::on_shutdown()
	{
		m_system_ticker_threadpool.shutdown();

		while (!m_levels.empty())
			destroy_level(*m_levels.back().get());
	}

	void Engine::create_level()
	{
		std::scoped_lock levels_lock(m_levels_mutex);

		assert(m_finished_setup && "Can't create a level before system has finished setup!");

		m_levels_to_add.push_back(std::make_unique<Level>(*this));

		Level& new_level = *m_levels_to_add.back().get();

		for (auto&& registration_callback : m_registration_callbacks)
			registration_callback(new_level);
	}

	void Engine::destroy_level(Level& inout_level)
	{
		std::scoped_lock levels_lock(m_levels_mutex);

		for (size_t i = 0; i < m_levels.size(); i++)
		{
			if (m_levels[i].get() != &inout_level)
				continue;

			for (auto& system : m_systems)
			{
				if (!is_shutting_down())
					system->on_end_simulation(Level_context(*this, inout_level));
			}

			m_levels.erase(m_levels.begin() + i);
			break;
		}
	}

	void Engine::refresh_time_delta()
	{
		const auto now = std::chrono::high_resolution_clock::now();
		const auto dif = now - m_previous_frame_time_point;
		m_time_delta = std::chrono::duration<float, std::milli>(dif).count() * 0.001f;
		m_previous_frame_time_point = now;
	}

	void Engine::flush_level_streaming()
	{
		//first remove levels
		for (Level* level : m_levels_to_remove)
		{
			for (auto& system : m_systems)
			{
				if (!is_shutting_down())
					system->on_end_simulation(Level_context(*this, *level));
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
					system->on_begin_simulation(Level_context(*this, *m_levels.back().get()));
			}
		}

		m_levels_to_add.clear();
	}

	std::unique_ptr<State>& Engine::get_state_at_index(i32 in_index)
	{
		while (in_index >= m_states.size())
			m_states.push_back(nullptr);

		return m_states[in_index];
	}
}
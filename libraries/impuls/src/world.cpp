#include "impuls/pch.h"
#include "impuls/world.h"
#include "impuls/object.h"
#include "impuls/world_context.h"

namespace impuls
{
	void world::initialize()
	{
		assert(!m_initialized && "");

		m_initialized = true;
		m_is_destroyed = false;

		refresh_time_delta();
	}

	void world::simulate()
	{
		refresh_time_delta();

		if (is_destroyed())
			return;

		if (!m_initialized)
			return;

		/*
			TODO:

			target flow:

			always tick all editor systems

			user needs to manually call begin_simulation();
			only tick() if m_begun_simulation = true
		*/

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

	void world::begin_simulation()
	{
		for (auto& system : m_systems)
		{
			if (!m_is_destroyed)
				system->on_begin_simulation(std::move(world_context(*this)));
		}

		m_begun_simulation = true;
	}

	void world::tick()
	{
		for (auto& system : m_systems)
		{
			if (!m_is_destroyed)
				system->on_tick(std::move(world_context(*this)), m_time_delta);
		}
	}

	void world::end_simulation()
	{
		for (auto& system : m_systems)
		{
			if (!m_is_destroyed)
				system->on_end_simulation(std::move(world_context(*this)));
		}
	}

	void world::refresh_time_delta()
	{
		const auto now = std::chrono::high_resolution_clock::now();
		const auto dif = now - m_previous_frame_time_point;
		m_time_delta = std::chrono::duration<float, std::milli>(dif).count() * 0.001f;
		m_previous_frame_time_point = now;
	}

	std::unique_ptr<component_storage>& world::get_component_storage_at_index(i32 in_index)
	{
		return m_component_storages[in_index];
	}

	std::unique_ptr<i_object_storage_base>& world::get_object_storage_at_index(i32 in_index)
	{
		while (in_index >= m_objects.size())
			m_objects.push_back(nullptr);

		return m_objects[in_index];
	}

	std::unique_ptr<i_state>& world::get_state_at_index(i32 in_index)
	{
		while (in_index >= m_states.size())
			m_states.push_back(nullptr);

		return m_states[in_index];
	}
}
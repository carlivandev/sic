#include "stdafx.h"
#include "world.h"

namespace impuls
{
	ui32 world_event_index::m_event_index_ticker = 0;

	void world::initialize(ui32 in_initial_object_capacity, ui32 in_object_bucket_capacity)
	{
		assert(!m_initialized && "");

		m_objects.allocate(in_initial_object_capacity, in_object_bucket_capacity);

		m_threadpool.spawn(8);

		m_initialized = true;
		m_is_destroyed = false;
	}

	void world::simulate()
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
			refresh_time_delta();
		}

		if (!is_destroyed())
			tick();

		if (is_destroyed())
			end_simulation();
	}

	void world::begin_simulation()
	{
		m_scheduler.finalize();

		for (auto& sys : m_systems)
		{
			auto it = m_scheduler.m_sys_to_item.find(sys.get());

			if (it != m_scheduler.m_sys_to_item.end())
			{
				for (auto* parallel_sys : it->second.m_can_run_in_parallel_with)
				{
					std::vector<std::string> failed_policy_reasons;
					const bool can_run_in_parallel_with = it->second.m_system->verify_policies_against(*parallel_sys, failed_policy_reasons);
					
					if (!can_run_in_parallel_with)
					{
						std::cout << "policies failed for: ";
						std::cout << "\"" << it->second.m_system->m_name << "\" and ";
						std::cout << "\"" << parallel_sys->m_name << "\"\n";

						
						for (auto& reason : failed_policy_reasons)
							std::cout << "\t- " << reason << "\n";
						
						destroy();
					}
				}
			}
		}

		for (auto& system : m_systems)
		{
			if (!m_is_destroyed)
				system->on_begin_simulation();
		}

		m_begun_simulation = true;
	}

	void world::tick()
	{
		bool work_to_do = true;

		while (work_to_do)
		{
			work_to_do = false;

			for (auto& it : m_scheduler.m_sys_to_item)
			{
				if (it.second.m_frame_state == scheduler::item::e_state::not_started)
				{
					bool can_exec = true;

					for (auto& dep : it.second.m_this_depends_on)
					{
						if (m_scheduler.m_sys_to_item[dep].m_frame_state != scheduler::item::e_state::finished)
						{
							can_exec = false;
							work_to_do = true;
							break;
						}
					}

					if (can_exec)
					{
						const float time_delta = m_time_delta;
						scheduler::item* sched_item = &it.second;

						m_threadpool.emplace
						(
							[time_delta, sched_item]()
							{
								sched_item->m_frame_state = scheduler::item::e_state::started;
								sched_item->m_system->on_tick(time_delta);
								sched_item->m_frame_state = scheduler::item::e_state::finished;
							}
						);

						work_to_do = true;
					}
				}
			}
		}

		bool all_work_finished = false;

		while (!all_work_finished)
		{
			all_work_finished = true;

			for (auto& it : m_scheduler.m_sys_to_item)
			{
				if (it.second.m_frame_state != scheduler::item::e_state::finished)
				{
					all_work_finished = false;
					break;
				}
			}
		}

		for (auto& it : m_scheduler.m_sys_to_item)
			it.second.m_frame_state = scheduler::item::e_state::not_started;
	}

	void world::end_simulation()
	{
		for (auto& system : m_systems)
		{
			if (!m_is_destroyed)
				system->on_end_simulation();
		}
	}

	object& world::create_object()
	{
		if (!m_object_free_indices.empty())
		{
			object& new_object = m_objects[m_object_free_indices.back()];
			new_object.m_instance_index = m_object_free_indices.back();

			m_object_free_indices.pop_back();

			return new_object;
		}

		object& new_object = m_objects.emplace_back();
		new (&new_object) object();

		new_object.m_instance_index = static_cast<int>(m_objects.size()) - 1;

		return new_object;
	}

	void world::destroy_object(object& in_object_to_destroy)
	{
		for (auto& component_handle : in_object_to_destroy.m_components)
		{
			const int instance_index = component_handle.m_component->m_instance_index;
			component_storage& storage = *m_component_storages[component_handle.m_type_index];
			byte& byte = storage.m_components[instance_index * storage.m_component_type_size];

			storage.m_free_indices.push_back(instance_index);

			i_component* component = reinterpret_cast<i_component*>(&byte);

			const ui32 event_index = m_component_type_index_to_destroyed_event_index_lut[component_handle.m_type_index];

			if (m_world_events[event_index])
				m_world_events[event_index]->invoke(component);

			component->~i_component();
			component->m_instance_index = -1;
			component->m_owner = nullptr;
		}

		m_object_free_indices.push_back(in_object_to_destroy.m_instance_index);

		in_object_to_destroy.m_components.clear();
		in_object_to_destroy.m_instance_index = -1;
	}

	void world::refresh_time_delta()
	{
		const auto now = std::chrono::high_resolution_clock::now();
		const auto dif = now - m_previous_frame_time_point;
		m_time_delta = std::chrono::duration<float, std::milli>(dif).count() * 0.001f;
		m_previous_frame_time_point = now;
	}
}
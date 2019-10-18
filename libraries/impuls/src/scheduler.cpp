#include "impuls/pch.h"
#include "impuls/scheduler.h"

void impuls::scheduler::schedule(i_system_base* in_to_schedule, std::vector<i_system_base*> in_dependencies)
{
	if (m_finalized)
	{
		std::cout << "failed to schedule item, scheduler has already been finalized!\n";
		return;
	}

	assert(m_sys_to_item.find(in_to_schedule) == m_sys_to_item.end() && "this system is already scheduled!");

	auto& it = m_sys_to_item[in_to_schedule];
	it.m_system = in_to_schedule;

	for (i_system_base* dependency : in_dependencies)
	{
		auto dep_it = m_sys_to_item.find(dependency);

		if (dep_it == m_sys_to_item.end())
		{
			std::cout << "need to add dependencies before the one who has the dependency!\n";
			return;
		}

		it.m_this_depends_on.push_back(dependency);
		dep_it->second.m_depends_on_this.push_back(in_to_schedule);
	}
}

void impuls::scheduler::finalize()
{
	std::set<std::pair<i_system_base*, i_system_base*>> verified_combinations;

	for (auto& it : m_sys_to_item)
		validate_item(it.second, verified_combinations);

	m_finalized = true;
}

void impuls::scheduler::reset()
{
	m_finalized = false;
	m_sys_to_item.clear();
}

void impuls::scheduler::travel_up(item& in_root, item& in_item, std::unordered_set<item*>& out_tree)
{
	for (i_system_base* up : in_item.m_this_depends_on)
	{
		auto it = m_sys_to_item.find(up);

		if (it == m_sys_to_item.end())
		{
			std::cout << "unassigned system found, abort!\n";
			return;
		}

		if (&it->second == &in_root)
		{
			std::cout << "recursion found, abort!\n";
			return;
		}

		out_tree.insert(&it->second);
		travel_up(in_root, it->second, out_tree);
	}
}

void impuls::scheduler::travel_down(item& in_root, item& in_item, std::unordered_set<item*>& out_tree)
{
	for (i_system_base* down : in_item.m_depends_on_this)
	{
		auto it = m_sys_to_item.find(down);

		if (it == m_sys_to_item.end())
		{
			std::cout << "unassigned system found, abort!\n";
			return;
		}

		if (&it->second == &in_root)
		{
			std::cout << "recursion found, abort!\n";
			return;
		}
		
		out_tree.insert(&it->second);
		travel_down(in_root, it->second, out_tree);
	}
}

void impuls::scheduler::validate_item(item& in_item, std::set<std::pair<i_system_base*, i_system_base*>>& in_verified_combinations)
{
	std::unordered_set<item*> items_in_tree;

	travel_up(in_item, in_item, items_in_tree);
	travel_down(in_item, in_item, items_in_tree);

	for (auto& it : m_sys_to_item)
	{
		std::pair<i_system_base*, i_system_base*> first_combination{ in_item.m_system, it.second.m_system };
		std::pair<i_system_base*, i_system_base*> second_combination{ it.second.m_system, in_item.m_system };

		if (items_in_tree.find(&it.second) == items_in_tree.end() &&
			in_item.m_system != it.second.m_system &&
			in_verified_combinations.find(first_combination) == in_verified_combinations.end() &&
			in_verified_combinations.find(second_combination) == in_verified_combinations.end())
		{
			//these can run in parallel
			in_verified_combinations.insert(first_combination);

			in_item.m_can_run_in_parallel_with.push_back(it.second.m_system);
			it.second.m_can_run_in_parallel_with.push_back(in_item.m_system);
		}
	}
}

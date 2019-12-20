#pragma once
#include <unordered_set>
#include <unordered_map>
#include <set>

namespace impuls
{
	struct i_system_base;

	struct Scheduler
	{
		struct Item
		{
			enum class e_state
			{
				not_started,
				started,
				finished
			};
			i_system_base* m_system;
			std::vector<i_system_base*> m_this_depends_on;
			std::vector<i_system_base*> m_depends_on_this;
			std::vector<i_system_base*> m_can_run_in_parallel_with;

			e_state m_frame_state = e_state::not_started;
		};

		void schedule(i_system_base* in_to_schedule, std::vector<i_system_base*> in_dependencies);
		void finalize();
		void reset();

		void travel_up(Item& in_root, Item& in_item, std::unordered_set<Item*>& out_tree);
		void travel_down(Item& in_root, Item& in_item, std::unordered_set<Item*>& out_tree);

		void validate_item(Item& in_item, std::set<std::pair<i_system_base*, i_system_base*>>& in_verified_combinations);

		std::unordered_map<i_system_base*, Item> m_sys_to_item;
		bool m_finalized = false;
	};
}

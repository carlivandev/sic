#pragma once
#include <unordered_set>
#include <set>

namespace impuls
{
	struct i_system_base;

	struct scheduler
	{
		struct item
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

		void travel_up(item& in_root, item& in_item, std::unordered_set<item*>& out_tree);
		void travel_down(item& in_root, item& in_item, std::unordered_set<item*>& out_tree);

		void validate_item(item& in_item, std::set<std::pair<i_system_base*, i_system_base*>>& in_verified_combinations);

		std::unordered_map<i_system_base*, item> m_sys_to_item;
		bool m_finalized = false;
	};
}

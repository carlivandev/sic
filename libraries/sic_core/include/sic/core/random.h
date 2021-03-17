#pragma once
#include "defines.h"

#include <random>
#include <algorithm>
#include <optional>
#include <unordered_set>

namespace sic
{
	struct Random
	{
		template <typename T_data_entry_type, typename T_roll_type = i32>
		struct Roll_table
		{
			struct Item
			{
				T_data_entry_type m_data;
				T_roll_type m_min = { 0 };
				T_roll_type m_max = { 0 };
			};

			void add_item(T_data_entry_type in_data, T_roll_type in_min, T_roll_type in_max)
			{
				Item new_item;
				new_item.m_data = in_data;
				new_item.m_min = in_min;
				new_item.m_max = in_max;
				m_items.push_back(new_item);
			}

			std::optional<T_data_entry_type> roll(T_roll_type in_roll_value) const
			{
				for (auto&& possible_roll : m_items)
					if (in_roll_value >= possible_roll.m_min && in_roll_value <= possible_roll.m_max)
						return possible_roll.m_data;

				return {};
			}

		private:
			std::vector<Item> m_items;
		};

		//[in_min, in_max)
		static float get(float in_min, float in_max);

		//[in_min, in_max]
		static i32 get(i32 in_min, i32 in_max);

		//returns true if in_x is [0, in_x], where in_max is the highest roll possible
		//ex. in_x = 25, in_max = 100, has 25% chance to return true
		static bool roll(i32 in_x, i32 in_max = 100);

		template <typename T_iterator_type>
		static void shuffle(T_iterator_type in_begin, T_iterator_type in_end);

		static std::mt19937& get_engine() { return m_engine; }

	private:
		static thread_local std::random_device m_device;
		static thread_local std::mt19937 m_engine;
	};
	template<typename T_iterator_type>
	inline void Random::shuffle(T_iterator_type in_begin, T_iterator_type in_end)
	{
		std::shuffle(in_begin, in_end, m_engine);
	}
}
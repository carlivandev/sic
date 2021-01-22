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
		template <typename T_data_entry_type>
		struct Roll_table
		{
			struct Chance_modifier
			{
				std::string m_tag;
				float m_modifier = 0.0f;
			};

			struct Item
			{
				std::vector<Chance_modifier> m_chance_modifiers;
				T_data_entry_type m_data;
				float m_chance = 0.0f;
			};

			void add_item(T_data_entry_type in_data, float in_chance, const std::vector<Chance_modifier>& in_chance_modifiers = {})
			{
				Item new_item;
				new_item.m_chance_modifiers = in_chance_modifiers;
				new_item.m_data = in_data;
				new_item.m_chance = in_chance;
				m_items.push_back(new_item);
			}

			std::optional<T_data_entry_type> roll(const std::unordered_set<std::string>& in_modifier_tags = {}) const
			{
				std::vector<Item> items_copy = m_items;

				for (auto&& item : items_copy)
				{
					for (auto&& modifier : item.m_chance_modifiers)
					{
						if (in_modifier_tags.contains(modifier.m_tag))
						{
							for (auto&& other_item : items_copy)
							{
								if (&item != &other_item)
									other_item.m_chance -= modifier.m_modifier / static_cast<float>(items_copy.size() - 1);
							}

							item.m_chance += modifier.m_modifier;
						}
					}
				}


				struct Roll_item
				{
					Item* m_item = nullptr;
					float m_min_chance;
					float m_max_chance;
				};

				std::vector<Roll_item> possible_rolls;

				float cur_roll_offset = 0.0f;
				for (auto& item : items_copy)
				{
					possible_rolls.push_back({ &item, cur_roll_offset, cur_roll_offset + item.m_chance });
					cur_roll_offset += item.m_chance;
				}

				const float result = sic::Random::get(0.0f, 1.0f);

				for (auto&& possible_roll : possible_rolls)
					if (result >= possible_roll.m_min_chance && result <= possible_roll.m_max_chance)
						return possible_roll.m_item->m_data;

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
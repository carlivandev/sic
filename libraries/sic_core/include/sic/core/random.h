#pragma once
#include "defines.h"

#include <random>

namespace sic
{
	struct Random
	{
		//[in_min, in_max)
		static float get(float in_min, float in_max);

		//[in_min, in_max]
		static i32 get(i32 in_min, i32 in_max);

		//returns true if in_x is [0, in_x], where in_max is the highest roll possible
		//ex. in_x = 25, in_max = 100, has 25% chance to return true
		static bool roll(i32 in_x, i32 in_max = 100);

	private:
		static thread_local std::random_device m_device;
		static thread_local std::mt19937 m_engine;
	};
}
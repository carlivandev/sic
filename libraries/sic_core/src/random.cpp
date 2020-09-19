#include "sic/core/random.h"

namespace sic
{
	thread_local std::random_device Random::m_device;

#ifdef _DEBUG
	thread_local std::mt19937 Random::m_engine(0);
#else
	thread_local std::mt19937 Random::m_engine(Random::m_device());
#endif

	 float Random::get(float in_min, float in_max)
	 {
		 std::uniform_real_distribution<float> dist(in_min, in_max);
		 return dist(m_engine);
	 }

	 i32 Random::get(i32 in_min, i32 in_max)
	 {
		 std::uniform_int_distribution<i32> dist(in_min, in_max);
		 return dist(m_engine);
	 }

	 bool Random::roll(i32 in_x, i32 in_max)
	 {
		 std::uniform_int_distribution<i32> dist(1, in_max);
		 return in_x < dist(m_engine);
	 }
}
#pragma once
#include "glm/common.hpp"

namespace sic
{
	struct Ease_function
	{
		static float Cubic_out(float in_x)
		{
			const float val = 1.0f - in_x;
			return 1.0f - (val * val * val);
		}

		static float Cubic_in(float in_x)
		{
			return in_x * in_x * in_x;
		}

		static float Cubic_inout(float in_x)
		{
			const float val = -2.0f * in_x + 2.0f;
			return in_x < 0.5f ? 4 * in_x * in_x * in_x : 1.0f - (val * val * val) / 2.0f;
		}

		static float Linear(float in_x)
		{
			return in_x;
		}
	};

	template <typename T = float>
	struct Ease
	{
		using Function_signature = float(*)(float);

		Ease() = default;
		Ease(const T& in_start, const T& in_target, float in_duration, Function_signature in_func) :
			m_start(in_start), m_target(in_target), m_duration(in_duration), m_cur_time(0.0f), m_func(in_func)
		{

		}

		void set(const T& in_start, const T& in_target, float in_duration, Function_signature in_func)
		{
			m_start = in_start;
			m_target = in_target;
			m_duration = in_duration;
			m_cur_time = 0.0f;

			m_func = in_func;
		}

		T tick(float in_dt)
		{
			m_cur_time += in_dt;
			m_cur_time = (glm::min)(m_duration, m_cur_time);

			const float t = m_cur_time / m_duration;
			return m_current_value = glm::mix(m_start, m_target, m_func(t));
		}

		T m_start = T();
		T m_target = T();
		float m_cur_time = 0.0f;
		float m_duration = 0.0f;
		T m_current_value = T();

		Function_signature m_func = nullptr;
	};
}
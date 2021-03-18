#pragma once
#include "sic/core/defines.h"
#include "glm/ext/vector_float4.hpp"

#include <vector>

namespace sic
{
	struct Color
	{
		Color() = default;
		Color(const std::vector<sic::ui8>& in_vec)
		{
			assert(in_vec.size() == 4);
			m_r = in_vec[0];
			m_g = in_vec[1];
			m_b = in_vec[2];
			m_a = in_vec[3];
		}

		Color(sic::ui8 in_r, sic::ui8 in_g, sic::ui8 in_b, sic::ui8 in_a) : m_r(in_r), m_g(in_g), m_b(in_b), m_a(in_a) {}

		glm::vec4 to_vec4() const
		{
			return { (float)m_r / 255.0f, (float)m_g / 255.0f, (float)m_b / 255.0f, (float)m_a / 255.0f };
		}

		sic::ui8 m_r = 0;
		sic::ui8 m_g = 0;
		sic::ui8 m_b = 0;
		sic::ui8 m_a = 0;
	};
}
#pragma once
#include "sic/core/defines.h"
#include "glm/ext/vector_float4.hpp"

#include <vector>

namespace sic
{
	struct Color
	{
		static Color white() { return Color(255, 255, 255, 255); };
		static Color black() { return Color(0, 0, 0, 255); };

		static Color red() { return Color(255, 0, 0, 255); };
		static Color green() { return Color(0, 255, 0, 255); };
		static Color blue() { return Color(0, 0, 255, 255); };

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

		Color& r(sic::ui8 in_r) { m_r = in_r; return *this; }
		Color& g(sic::ui8 in_g) { m_g = in_g; return *this; }
		Color& b(sic::ui8 in_b) { m_b = in_b; return *this; }
		Color& a(sic::ui8 in_a) { m_a = in_a; return *this; }

		Color operator+(const Color& in_other) const
		{
			const Color ret_val
			(
				(ui8)glm::clamp((i16)m_r + (i16)in_other.m_r, 0, 255),
				(ui8)glm::clamp((i16)m_g + (i16)in_other.m_g, 0, 255),
				(ui8)glm::clamp((i16)m_b + (i16)in_other.m_b, 0, 255),
				(ui8)glm::clamp((i16)m_a + (i16)in_other.m_a, 0, 255)
			);

			return ret_val;
		}

		Color& operator+=(const Color& in_other)
		{
			m_r = (ui8)glm::clamp((i16)m_r + (i16)in_other.m_r, 0, 255);
			m_g = (ui8)glm::clamp((i16)m_g + (i16)in_other.m_g, 0, 255);
			m_b = (ui8)glm::clamp((i16)m_b + (i16)in_other.m_b, 0, 255);
			m_a = (ui8)glm::clamp((i16)m_a + (i16)in_other.m_a, 0, 255);

			return *this;
		}

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
#pragma once
#include "impuls/component.h"
#include "impuls/delegate.h"

#include "glm/vec3.hpp"

namespace impuls
{
	struct component_transform : public i_component
	{
		struct on_position_changed : delegate<glm::vec3> {};

		void set_position(const glm::vec3& in_position)
		{
			if (m_position != in_position)
			{
				m_position = in_position;
				m_on_position_changed.invoke(in_position);
			}
		}
		const glm::vec3& get_position() const { return m_position; }

		on_position_changed m_on_position_changed;

	protected:
		glm::vec3 m_position = {0.0f, 0.0f, 0.0f};
	};
}
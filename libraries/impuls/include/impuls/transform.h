#pragma once
#include "component.h"
#include "glm/vec3.hpp"

namespace impuls
{
	struct component_transform : public i_component
	{
		glm::vec3 m_position = {0.0f, 0.0f, 0.0f};
	};
}
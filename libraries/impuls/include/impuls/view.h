#pragma once
#include "impuls/component.h"

#include "impuls/gl_includes.h"
#include "impuls/object.h"

#include "glm/vec3.hpp"

namespace impuls
{
	struct object_window;

	struct component_view : public i_component
	{
		object_window* m_window_render_on = nullptr;

		// position
		glm::vec3 m_position = glm::vec3(0, 0, 5);
		// horizontal angle : toward -Z
		float m_horizontal_angle = 3.14f;
		// vertical angle : 0, look at the horizon
		float m_vertical_angle = 0.0f;
	};

	struct object_view : public i_object<object_view, component_view> {};

}
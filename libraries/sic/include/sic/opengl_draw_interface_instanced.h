#pragma once
#include "sic/defines.h"
#include "sic/type_restrictions.h"
#include "sic/asset_types.h"

#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

#include <vector>

namespace sic
{
	struct OpenGl_draw_interface_instanced : Noncopyable
	{
		void begin_frame(const Asset_model::Mesh& in_mesh, Asset_material& in_material);
		void draw_instance(const char* in_buffer_data);
		void end_frame();

	private:
		void flush();

		const Asset_model::Mesh* m_mesh = nullptr;
		Asset_material* m_material = nullptr;
		std::vector<char> m_instance_buffer;
		char* m_current_instance;
	};
}
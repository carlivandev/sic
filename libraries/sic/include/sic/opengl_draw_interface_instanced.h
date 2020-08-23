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
		void begin_frame(const OpenGl_vertex_buffer_array_base& in_vba, const OpenGl_buffer& in_index_buffer, Asset_material& in_material);
		void draw_instance(const char* in_buffer_data);
		void end_frame();

	private:
		void flush();

		const OpenGl_vertex_buffer_array_base* m_vba = nullptr;
		const OpenGl_buffer* m_index_buffer = nullptr;

		Asset_material* m_material = nullptr;
		std::vector<char> m_instance_buffer;
		char* m_current_instance;
	};
}
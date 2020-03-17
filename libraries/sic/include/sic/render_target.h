#pragma once
#include "sic/defines.h"
#include "sic/gl_includes.h"
#include "sic/type_restrictions.h"

#include "glm/vec2.hpp"

namespace sic
{
	struct Render_target
	{
		Render_target() = default;
		~Render_target();

		void initialize(const glm::ivec2& in_dimensions);

		void bind_as_target();
		void bind_as_texture(ui32 in_texture_index, ui32 in_uniform_location);
		
		ui32 m_frame_buffer_id = (std::numeric_limits<ui32>::max)();
		ui32 m_texture_id = (std::numeric_limits<ui32>::max)();
		ui32 m_depth_stencil_rb_id = (std::numeric_limits<ui32>::max)();
	};
}
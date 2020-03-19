#pragma once
#include "sic/defines.h"
#include "sic/gl_includes.h"
#include "sic/type_restrictions.h"

#include "glm/vec2.hpp"
#include "glm/vec4.hpp"

#include <optional>

namespace sic
{
	struct OpenGl_render_target : Noncopyable
	{
		OpenGl_render_target(const glm::ivec2& in_dimensions, bool in_depth_test);
		~OpenGl_render_target();

		OpenGl_render_target(OpenGl_render_target&& in_other) noexcept;
		OpenGl_render_target& operator=(OpenGl_render_target&& in_other)  noexcept;

		void bind_as_target(ui32 in_output_index);
		void bind_as_texture(ui32 in_texture_index, ui32 in_uniform_location);
		
		void clear(std::optional<glm::vec4> in_clear_color_override = {});

		void set_clear_color(const glm::vec4& in_clear_color) { m_clear_color = in_clear_color; }

		const glm::ivec2& get_dimensions() const { return m_dimensions; }

	private:
		void free_resources();

		glm::ivec2 m_dimensions = { 0, 0 };
		glm::vec4 m_clear_color = { 0.0f, 0.0f, 0.0f, 0.0f };
		ui32 m_frame_buffer_id = 0;
		ui32 m_texture_id = 0;
		ui32 m_depth_stencil_rb_id = 0;
	};
}
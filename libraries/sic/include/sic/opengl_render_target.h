#pragma once
#include "sic/gl_includes.h"
#include "sic/opengl_texture.h"

#include "sic/core/defines.h"
#include "sic/core/type_restrictions.h"

#include "glm/vec2.hpp"
#include "glm/vec4.hpp"

#include <optional>

namespace sic
{
	struct OpenGl_render_target : Noncopyable
	{
		struct Creation_params
		{
			glm::ivec2 m_dimensions = { 1.0f, 1.0f };
			OpenGl_texture_format m_texture_format = OpenGl_texture_format::invalid;
			bool m_depth_test = false;
		};

		OpenGl_render_target(const glm::ivec2& in_dimensions, OpenGl_texture_format in_texture_format, bool in_depth_test);
		OpenGl_render_target(const Creation_params& in_params);
		~OpenGl_render_target();

		OpenGl_render_target(OpenGl_render_target&& in_other) noexcept;
		OpenGl_render_target& operator=(OpenGl_render_target&& in_other)  noexcept;

		void bind_as_target(ui32 in_output_index) const;

		void clear(std::optional<glm::vec4> in_clear_color_override = {}) const;

		void set_clear_color(const glm::vec4& in_clear_color) { m_clear_color = in_clear_color; }

		void resize(const glm::ivec2& in_dimensions);

		const glm::ivec2& get_dimensions() const { return m_dimensions; }
		const OpenGl_texture& get_texture() const { return m_texture.value(); }

	private:
		void create_resources(const glm::ivec2& in_dimensions, OpenGl_texture_format in_texture_format, bool in_depth_test);
		void free_resources();

		std::optional<OpenGl_texture> m_texture;
		glm::ivec2 m_dimensions = { 0, 0 };
		glm::vec4 m_clear_color = { 0.0f, 0.0f, 0.0f, 0.0f };

		ui32 m_frame_buffer_id = 0;
		ui32 m_depth_stencil_rb_id = 0;
	};
}
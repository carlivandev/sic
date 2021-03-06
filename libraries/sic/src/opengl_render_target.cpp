#pragma once
#include "sic/opengl_render_target.h"

#include "sic/core/logger.h"

sic::OpenGl_render_target::OpenGl_render_target(const glm::ivec2& in_dimensions, OpenGl_texture_format in_texture_format, bool in_depth_test)
{
	create_resources(in_dimensions, in_texture_format, in_depth_test);
}

sic::OpenGl_render_target::OpenGl_render_target(const Creation_params& in_params)
{
	create_resources(in_params.m_dimensions, in_params.m_texture_format, in_params.m_depth_test);
}

sic::OpenGl_render_target::~OpenGl_render_target()
{
	free_resources();
}

sic::OpenGl_render_target::OpenGl_render_target(OpenGl_render_target&& in_other) noexcept
{
	free_resources();

	m_texture = std::move(in_other.m_texture);

	m_frame_buffer_id = in_other.m_frame_buffer_id;
	m_depth_stencil_rb_id = in_other.m_depth_stencil_rb_id;

	m_dimensions = in_other.m_dimensions;
	m_clear_color = in_other.m_clear_color;

	in_other.m_frame_buffer_id = 0;
	in_other.m_depth_stencil_rb_id = 0;
}

sic::OpenGl_render_target& sic::OpenGl_render_target::operator=(OpenGl_render_target&& in_other) noexcept
{
	free_resources();

	m_texture = std::move(in_other.m_texture);

	m_frame_buffer_id = in_other.m_frame_buffer_id;
	m_depth_stencil_rb_id = in_other.m_depth_stencil_rb_id;

	m_dimensions = in_other.m_dimensions;
	m_clear_color = in_other.m_clear_color;

	in_other.m_frame_buffer_id = 0;
	in_other.m_depth_stencil_rb_id = 0;

	return *this;
}

void sic::OpenGl_render_target::bind_as_target(ui32 in_output_index) const
{
	if (glCheckNamedFramebufferStatus(m_frame_buffer_id, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		SIC_LOG_E(g_log_renderer, "Failed to bind Render_target. Framebuffer status not complete!");
		return;
	}

	SIC_GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, m_frame_buffer_id));
	SIC_GL_CHECK(glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + in_output_index, m_texture.value().get_id(), 0));
}

void sic::OpenGl_render_target::clear(std::optional<glm::vec4> in_clear_color_override) const
{
	SIC_GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, m_frame_buffer_id));

	const glm::vec4 clear_color = in_clear_color_override.value_or(m_clear_color);
	glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);

	if (m_depth_stencil_rb_id != 0)
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	else
		glClear(GL_COLOR_BUFFER_BIT);
}

void sic::OpenGl_render_target::resize(const glm::ivec2& in_dimensions)
{
	if (m_dimensions.x != in_dimensions.x ||
		m_dimensions.y != in_dimensions.y)
	{
		const bool has_depth_test = m_depth_stencil_rb_id != 0;
		const OpenGl_texture_format format = m_texture->get_format();

		free_resources();
		create_resources(in_dimensions, format, has_depth_test);
	}
}

void sic::OpenGl_render_target::create_resources(const glm::ivec2& in_dimensions, OpenGl_texture_format in_texture_format, bool in_depth_test)
{
	m_dimensions = in_dimensions;

	OpenGl_texture::Creation_params_2D params;
	params.set_dimensions(m_dimensions);
	params.set_format(in_texture_format);
	params.set_channel_type(OpenGl_texture_channel_type::unsigned_byte);
	params.set_filtering(OpenGl_texture_mag_filter::linear, OpenGl_texture_min_filter::linear);

	m_texture.emplace(params);

	SIC_GL_CHECK(glGenFramebuffers(1, &m_frame_buffer_id));
	SIC_GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, m_frame_buffer_id));

	SIC_GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture.value().get_id(), 0));

	if (in_depth_test)
	{
		SIC_GL_CHECK(glGenRenderbuffers(1, &m_depth_stencil_rb_id));
		SIC_GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, m_depth_stencil_rb_id));
		SIC_GL_CHECK(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, in_dimensions.x, in_dimensions.y));
		SIC_GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, 0));

		SIC_GL_CHECK(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depth_stencil_rb_id));
	}

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		SIC_LOG_E(g_log_renderer, "Framebuffer status not complete!");

	//reset framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void sic::OpenGl_render_target::free_resources()
{
	if (m_frame_buffer_id != 0)
		SIC_GL_CHECK(glDeleteFramebuffers(1, &m_frame_buffer_id));

	if (m_depth_stencil_rb_id != 0)
		SIC_GL_CHECK(glDeleteRenderbuffers(1, &m_depth_stencil_rb_id));

	m_texture.reset();
}

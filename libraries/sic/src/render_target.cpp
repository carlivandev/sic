#pragma once
#include "sic/Render_target.h"
#include "sic/logger.h"

sic::Render_target::~Render_target()
{
	if (m_frame_buffer_id != (std::numeric_limits<ui32>::max)())
		glDeleteFramebuffers(1, &m_frame_buffer_id);

	if (m_texture_id != (std::numeric_limits<ui32>::max)())
		glDeleteTextures(1, &m_texture_id);

	if (m_depth_stencil_rb_id != (std::numeric_limits<ui32>::max)())
	glDeleteRenderbuffers(1, &m_depth_stencil_rb_id);
}

void sic::Render_target::initialize(const glm::ivec2& in_dimensions)
{
	glGenFramebuffers(1, &m_frame_buffer_id);

	glBindFramebuffer(GL_FRAMEBUFFER, m_frame_buffer_id);

	glGenTextures(1, &m_texture_id);
	glBindTexture(GL_TEXTURE_2D, m_texture_id);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, in_dimensions.x, in_dimensions.y, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture_id, 0);

	if (true)
	{
		glGenRenderbuffers(1, &m_depth_stencil_rb_id);
		glBindRenderbuffer(GL_RENDERBUFFER, m_depth_stencil_rb_id);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, in_dimensions.x, in_dimensions.y);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depth_stencil_rb_id);
	}

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		SIC_LOG_E(g_log_renderer, "Framebuffer status not complete!");

	//reset framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void sic::Render_target::bind_as_target()
{
	if (glCheckNamedFramebufferStatus(m_frame_buffer_id, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		SIC_LOG_E(g_log_renderer, "Failed to bind Render_target. Framebuffer status not complete!");
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, m_frame_buffer_id);
}

void sic::Render_target::bind_as_texture(ui32 in_texture_index, ui32 in_uniform_location)
{
	glActiveTexture(GL_TEXTURE0 + in_texture_index);
	glBindTexture(GL_TEXTURE_2D, m_texture_id);
	glUniform1i(in_uniform_location, in_texture_index);
}

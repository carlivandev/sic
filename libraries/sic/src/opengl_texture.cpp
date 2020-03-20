#pragma once
#include "sic/opengl_texture.h"

sic::OpenGl_texture::OpenGl_texture(const glm::ivec2& in_dimensions, GLuint in_channel_format, GLuint in_mag_filter, GLuint in_min_filter, unsigned char* in_data)
{
	glGenTextures(1, &m_texture_id);
	m_dimension_format = GL_TEXTURE_2D;
	m_channel_format = in_channel_format;

	glBindTexture(m_dimension_format, m_texture_id);

	// Nice trilinear filtering.
	glTexParameteri(m_dimension_format, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(m_dimension_format, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(m_dimension_format, GL_TEXTURE_MAG_FILTER, in_mag_filter);
	glTexParameteri(m_dimension_format, GL_TEXTURE_MIN_FILTER, in_min_filter);

	glTexImage2D(m_dimension_format, 0, m_channel_format, in_dimensions.x, in_dimensions.y, 0, m_channel_format, GL_UNSIGNED_BYTE, in_data);

	glGenerateMipmap(m_dimension_format);

	glBindTexture(m_dimension_format, 0);
}

sic::OpenGl_texture::~OpenGl_texture()
{
	free_resources();
}

sic::OpenGl_texture::OpenGl_texture(OpenGl_texture&& in_other) noexcept
{
	free_resources();

	m_target_type = in_other.m_target_type;
	m_dimension_format = in_other.m_dimension_format;
	m_channel_format = in_other.m_channel_format;

	m_texture_id = in_other.m_texture_id;
	in_other.m_texture_id = 0;
}

sic::OpenGl_texture& sic::OpenGl_texture::operator=(OpenGl_texture&& in_other) noexcept
{
	free_resources();

	m_target_type = in_other.m_target_type;
	m_dimension_format = in_other.m_dimension_format;
	m_channel_format = in_other.m_channel_format;

	m_texture_id = in_other.m_texture_id;
	in_other.m_texture_id = 0;

	return *this;
}

void sic::OpenGl_texture::bind(GLuint in_uniform_location, GLuint in_active_texture_index)
{
	glActiveTexture(GL_TEXTURE0 + in_active_texture_index);
	glBindTexture(GL_TEXTURE_2D, m_texture_id);
	glUniform1i(in_uniform_location, in_active_texture_index);
}

void sic::OpenGl_texture::free_resources()
{
	if (m_texture_id != 0)
	{
		glDeleteTextures(1, &m_texture_id);
		m_texture_id = 0;
	}
}

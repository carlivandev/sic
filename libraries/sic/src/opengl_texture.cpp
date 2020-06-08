#pragma once
#include "sic/opengl_texture.h"

sic::OpenGl_texture::OpenGl_texture(const glm::ivec2& in_dimensions, GLuint in_channel_format, GLuint in_mag_filter, GLuint in_min_filter, unsigned char* in_data)
{
	glGenTextures(1, &m_texture_id);

	m_dimension_format = GL_TEXTURE_2D;
	m_channel_format = in_channel_format;
	m_channel_type = GL_UNSIGNED_BYTE;

	glBindTexture(m_dimension_format, m_texture_id);

	// Nice trilinear filtering.
	glTexParameteri(m_dimension_format, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(m_dimension_format, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(m_dimension_format, GL_TEXTURE_MAG_FILTER, in_mag_filter);
	glTexParameteri(m_dimension_format, GL_TEXTURE_MIN_FILTER, in_min_filter);

	glTexImage2D(m_dimension_format, 0, m_channel_format, in_dimensions.x, in_dimensions.y, 0, m_channel_format, m_channel_type, in_data);

	glGenerateMipmap(m_dimension_format);

	glBindTexture(m_dimension_format, 0);

	SIC_GL_CHECK((m_bindless_handle = glGetTextureHandleARB(m_texture_id)));
	SIC_GL_CHECK(glMakeTextureHandleResidentARB(m_bindless_handle));
}

sic::OpenGl_texture::OpenGl_texture(const glm::ivec2& in_dimensions, GLuint in_internal_channel_format, GLuint in_channel_format, GLuint in_channel_type, GLuint in_mag_filter, GLuint in_min_filter, unsigned char* in_data)
{
	glGenTextures(1, &m_texture_id);

	m_dimension_format = GL_TEXTURE_2D;
	m_channel_format = in_channel_format;
	m_channel_type = in_channel_type;

	glBindTexture(m_dimension_format, m_texture_id);
	glObjectLabel(GL_TEXTURE, m_texture_id, -1, "Example Texture");

	// Nice trilinear filtering.
	glTexParameteri(m_dimension_format, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(m_dimension_format, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(m_dimension_format, GL_TEXTURE_MAG_FILTER, in_mag_filter);
	glTexParameteri(m_dimension_format, GL_TEXTURE_MIN_FILTER, in_min_filter);

	glTexImage2D(m_dimension_format, 0, in_internal_channel_format, in_dimensions.x, in_dimensions.y, 0, m_channel_format, m_channel_type, in_data);

	glGenerateMipmap(m_dimension_format);

	glBindTexture(m_dimension_format, 0);

	SIC_GL_CHECK((m_bindless_handle = glGetTextureHandleARB(m_texture_id)));
	SIC_GL_CHECK(glMakeTextureHandleResidentARB(m_bindless_handle));
}

sic::OpenGl_texture::OpenGl_texture(const OpenGl_buffer& in_buffer)
{
	m_dimension_format = GL_TEXTURE_BUFFER;
	m_channel_format = GL_RGBA32F;

	glGenTextures(1, &m_texture_id);
	glActiveTexture(GL_TEXTURE0);
	SIC_GL_CHECK(glBindTexture(m_dimension_format, m_texture_id));
	SIC_GL_CHECK(glTexBuffer(m_dimension_format, m_channel_format, in_buffer.get_buffer_id()));
	SIC_GL_CHECK(glBindTexture(m_dimension_format, 0));
	SIC_GL_CHECK((m_bindless_handle = glGetTextureHandleARB(m_texture_id)));
	SIC_GL_CHECK(glMakeTextureHandleResidentARB(m_bindless_handle));
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
	m_channel_type = in_other.m_channel_type;

	m_texture_id = in_other.m_texture_id;
	m_bindless_handle = in_other.m_bindless_handle;

	in_other.m_texture_id = 0;
}

sic::OpenGl_texture& sic::OpenGl_texture::operator=(OpenGl_texture&& in_other) noexcept
{
	free_resources();

	m_target_type = in_other.m_target_type;
	m_dimension_format = in_other.m_dimension_format;
	m_channel_format = in_other.m_channel_format;
	m_channel_type = in_other.m_channel_type;

	m_texture_id = in_other.m_texture_id;
	m_bindless_handle = in_other.m_bindless_handle;

	in_other.m_texture_id = 0;

	return *this;
}

void sic::OpenGl_texture::update_data(const glm::ivec2& in_offset, const glm::ivec2& in_dimensions, void* in_data)
{
	glBindTexture(m_dimension_format, m_texture_id);

	glTexSubImage2D
	(
		m_dimension_format,
		0,
		in_offset.x,
		in_offset.y,
		in_dimensions.x,
		in_dimensions.y,
		m_channel_format,
		m_channel_type,
		in_data
	);

	glBindTexture(m_dimension_format, 0);
}

void sic::OpenGl_texture::free_resources()
{
	if (m_texture_id != 0)
	{
		glDeleteTextures(1, &m_texture_id);
		m_texture_id = 0;
	}
}

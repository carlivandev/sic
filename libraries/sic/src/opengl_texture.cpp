#pragma once
#include "sic/opengl_texture.h"

sic::OpenGl_texture::OpenGl_texture(const OpenGl_texture::Creation_params_2D& in_params)
{
	m_dimension_format = GL_TEXTURE_2D;

	begin_initialize_texture_params(in_params);
	glTexImage2D(m_dimension_format, 0, static_cast<GLint>(in_params.m_internal_format), in_params.m_dimensions.x, in_params.m_dimensions.y, 0, static_cast<GLenum>(in_params.m_format), static_cast<GLenum>(m_channel_type), in_params.m_data);
	end_initialize_texture_params(in_params);
}

sic::OpenGl_texture::OpenGl_texture(const Creation_params_buffer& in_params)
{
	m_dimension_format = GL_TEXTURE_BUFFER;
	m_format = in_params.m_format;
	
	glGenTextures(1, &m_texture_id);
	SIC_GL_CHECK(glBindTexture(m_dimension_format, m_texture_id));

	if (!in_params.m_debug_name.empty())
		glObjectLabel(GL_TEXTURE, m_texture_id, -1, in_params.m_debug_name.c_str());

	SIC_GL_CHECK(glTexBuffer(m_dimension_format, static_cast<GLenum>(in_params.m_internal_format), in_params.m_buffer->get_buffer_id()));
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
	m_format = in_other.m_format;
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
	m_format = in_other.m_format;
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
		static_cast<GLenum>(m_format),
		static_cast<GLenum>(m_channel_type),
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

void sic::OpenGl_texture::begin_initialize_texture_params(const Creation_params_texture_base& in_params)
{
	glGenTextures(1, &m_texture_id);

	m_format = in_params.m_format;
	m_channel_type = in_params.m_channel_type;

	glBindTexture(m_dimension_format, m_texture_id);

	if (!in_params.m_debug_name.empty())
		glObjectLabel(GL_TEXTURE, m_texture_id, -1, in_params.m_debug_name.c_str());

	// Nice trilinear filtering.
	glTexParameteri(m_dimension_format, GL_TEXTURE_WRAP_S, static_cast<GLint>(in_params.m_wrap_mode));
	glTexParameteri(m_dimension_format, GL_TEXTURE_WRAP_T, static_cast<GLint>(in_params.m_wrap_mode));

	glTexParameteri(m_dimension_format, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(in_params.m_mag_filter));
	glTexParameteri(m_dimension_format, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(in_params.m_min_filter));
}

void sic::OpenGl_texture::end_initialize_texture_params(const Creation_params_texture_base&)
{
	//TODO (Carl H): make this optional
	glGenerateMipmap(m_dimension_format);

	glBindTexture(m_dimension_format, 0);

	SIC_GL_CHECK((m_bindless_handle = glGetTextureHandleARB(m_texture_id)));
	SIC_GL_CHECK(glMakeTextureHandleResidentARB(m_bindless_handle));
}
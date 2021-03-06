#pragma once
#include "sic/opengl_program.h"
#include "sic/opengl_texture.h"

#include "sic/core/logger.h"

#include "glm/gtc/type_ptr.hpp"

#include <vector>

GLint sic::OpenGl_program::m_currently_enabled_attribute_count = 0;

sic::OpenGl_program::OpenGl_program(const std::string& in_vertex_shader_name, const std::string& in_vertex_shader_code, const std::string& in_fragment_shader_name, const std::string& in_fragment_shader_code)
{
	m_is_valid = true;

	// Create the shaders
	GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

	GLint result_id = GL_FALSE;
	i32 info_log_length;

	// Compile Vertex Shader
	SIC_LOG(g_log_renderer_verbose, "Compiling shader : {0}", in_vertex_shader_name.c_str());

	const GLchar* vertex_source_ptr = in_vertex_shader_code.data();
	glShaderSource(vertex_shader_id, 1, &vertex_source_ptr, NULL);
	glCompileShader(vertex_shader_id);

	// Check Vertex Shader
	glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &result_id);
	glGetShaderiv(vertex_shader_id, GL_INFO_LOG_LENGTH, &info_log_length);
	if (info_log_length > 0)
	{
		std::vector<char> vertex_shader_error_message(static_cast<size_t>(info_log_length) + 1);
		glGetShaderInfoLog(vertex_shader_id, info_log_length, NULL, &vertex_shader_error_message[0]);
		SIC_LOG_E(g_log_renderer, &vertex_shader_error_message[0]);

		m_is_valid = false;
	}

	// Compile Fragment Shader
	SIC_LOG(g_log_renderer_verbose, "Compiling shader : {0}", in_fragment_shader_name.c_str());

	const GLchar* fragment_source_ptr = in_fragment_shader_code.data();
	glShaderSource(fragment_shader_id, 1, &fragment_source_ptr, NULL);
	glCompileShader(fragment_shader_id);

	// Check Fragment Shader
	glGetShaderiv(fragment_shader_id, GL_COMPILE_STATUS, &result_id);
	glGetShaderiv(fragment_shader_id, GL_INFO_LOG_LENGTH, &info_log_length);
	if (info_log_length > 0)
	{
		std::vector<char> fragment_shader_error_message(static_cast<size_t>(info_log_length) + 1);
		glGetShaderInfoLog(fragment_shader_id, info_log_length, NULL, &fragment_shader_error_message[0]);
		SIC_LOG_E(g_log_renderer, "Error compiling fragment shader with path: \"{0}\"! Error: {1}", in_fragment_shader_name, std::string_view(&fragment_shader_error_message[0], fragment_shader_error_message.size()));

		m_is_valid = false;
	}

	// Link the program
	SIC_LOG(g_log_renderer_verbose, "Linking program");
	m_program_id = glCreateProgram();
	glAttachShader(m_program_id, vertex_shader_id);
	glAttachShader(m_program_id, fragment_shader_id);
	glLinkProgram(m_program_id);

	// Check the program
	glGetProgramiv(m_program_id, GL_LINK_STATUS, &result_id);
	glGetProgramiv(m_program_id, GL_INFO_LOG_LENGTH, &info_log_length);
	if (info_log_length > 0)
	{
		std::vector<char> program_error_message(static_cast<size_t>(info_log_length) + 1);
		glGetProgramInfoLog(m_program_id, info_log_length, NULL, &program_error_message[0]);
		SIC_LOG_E(g_log_renderer, "Error linking program: {0}", std::string_view(&program_error_message[0], program_error_message.size()));

		m_is_valid = false;
	}

	glDetachShader(m_program_id, vertex_shader_id);
	glDetachShader(m_program_id, fragment_shader_id);

	glDeleteShader(vertex_shader_id);
	glDeleteShader(fragment_shader_id);

	std::vector<GLchar> name_data(256);
	std::vector<GLenum> properties;
	properties.push_back(GL_NAME_LENGTH);
	properties.push_back(GL_TYPE);
	properties.push_back(GL_ARRAY_SIZE);
	properties.push_back(GL_BLOCK_INDEX);
	std::vector<GLint> values(properties.size());

	GLint num_active_attribs = 0;
	glGetProgramInterfaceiv(m_program_id, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES, &num_active_attribs);

	GLint num_active_uniforms = 0;
	glGetProgramInterfaceiv(m_program_id, GL_UNIFORM, GL_ACTIVE_RESOURCES, &num_active_uniforms);

	GLint num_active_uniform_blocks = 0;
	glGetProgramInterfaceiv(m_program_id, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES, &num_active_uniform_blocks);

	for (GLint attrib = 0; attrib < num_active_attribs; ++attrib)
	{
		glGetProgramResourceiv(m_program_id, GL_PROGRAM_INPUT, attrib, (GLsizei)properties.size() - 1,
			&properties[0], (GLsizei)values.size(), NULL, &values[0]);

		name_data.resize(values[0]); //The length of the name.
		glGetProgramResourceName(m_program_id, GL_PROGRAM_INPUT, attrib, (GLsizei)name_data.size(), NULL, &name_data[0]);
		std::string name((char*)&name_data[0], name_data.size() - 1);

		if (name.size() > 2 && name[0] == 'g' && name[1] == 'l' && name[2] == '_')
			continue;

		m_attributes.push_back(name);
	}

	m_attribute_count = (GLsizei)m_attributes.size();

	for (GLuint uniform_idx = 0; uniform_idx < static_cast<GLuint>(num_active_uniforms); ++uniform_idx)
	{
		SIC_GL_CHECK(glGetProgramResourceiv(m_program_id, GL_UNIFORM, uniform_idx, static_cast<GLsizei>(properties.size()),
			&properties[0], static_cast<GLsizei>(values.size()), NULL, &values[0]));

		name_data.resize(values[0]);
		SIC_GL_CHECK(glGetProgramResourceName(m_program_id, GL_UNIFORM, uniform_idx, static_cast<GLsizei>(name_data.size()), NULL, &name_data[0]));
		std::string name((char*)& name_data[0], name_data.size() - 1);

		const bool is_uniform_block_variable = values[3] != -1;
		if (!is_uniform_block_variable)
			m_uniforms[name] = glGetUniformLocation(m_program_id, name.c_str());
	}

	std::vector<GLenum> uniform_block_properties;
	uniform_block_properties.push_back(GL_NAME_LENGTH);

	for (GLuint uniform_block_idx = 0; uniform_block_idx < static_cast<GLuint>(num_active_uniform_blocks); ++uniform_block_idx)
	{
		SIC_GL_CHECK(glGetProgramResourceiv(m_program_id, GL_UNIFORM_BLOCK, uniform_block_idx, static_cast<GLsizei>(uniform_block_properties.size()),
			&uniform_block_properties[0], 1, NULL, &values[0]));
		name_data.resize(values[0]);
		SIC_GL_CHECK(glGetProgramResourceName(m_program_id, GL_UNIFORM_BLOCK, uniform_block_idx, static_cast<GLsizei>(name_data.size()), NULL, &name_data[0]));
		std::string name((char*)& name_data[0], name_data.size() - 1);
		m_uniform_blocks[name] = glGetUniformBlockIndex(m_program_id, name.c_str());
	}
}

sic::OpenGl_program::~OpenGl_program()
{
	free_resources();
}

sic::OpenGl_program::OpenGl_program(OpenGl_program&& in_other) noexcept
{
	free_resources();

	m_program_id = in_other.m_program_id;
	m_attribute_count = in_other.m_attribute_count;
	m_attributes = std::move(in_other.m_attributes);
	m_uniforms = std::move(in_other.m_uniforms);
	m_uniform_blocks = std::move(in_other.m_uniform_blocks);

	in_other.m_program_id = 0;
}

sic::OpenGl_program& sic::OpenGl_program::operator=(OpenGl_program&& in_other) noexcept
{
	free_resources();

	m_program_id = in_other.m_program_id;
	m_attribute_count = in_other.m_attribute_count;
	m_attributes = std::move(in_other.m_attributes);
	m_uniforms = std::move(in_other.m_uniforms);
	m_uniform_blocks = std::move(in_other.m_uniform_blocks);

	in_other.m_program_id = 0;

	return *this;
}

void sic::OpenGl_program::use() const
{
	for (GLint i = m_currently_enabled_attribute_count; i < m_attribute_count; i++)
		glEnableVertexAttribArray(i);
	
	for (GLint i = m_attribute_count; i < m_currently_enabled_attribute_count; i++)
		glDisableVertexAttribArray(i);

	m_currently_enabled_attribute_count = m_attribute_count;

	SIC_GL_CHECK(glUseProgram(m_program_id));
}

bool sic::OpenGl_program::set_uniform(const char* in_key, const glm::mat4& in_matrix) const
{
	auto it = m_uniforms.find(in_key);

	if (it == m_uniforms.end())
	{
		SIC_LOG_W(g_log_renderer_verbose, "Failed to set mat4 uniform with name: \"{0}\"", in_key);
		return false;
	}

	SIC_GL_CHECK(glUniformMatrix4fv(it->second, 1, GL_FALSE, glm::value_ptr(in_matrix)));
	return true;
}

bool sic::OpenGl_program::set_uniform(const char* in_key, const glm::vec3& in_vec3) const
{
	auto it = m_uniforms.find(in_key);

	if (it == m_uniforms.end())
	{
		SIC_LOG_W(g_log_renderer_verbose, "Failed to set vec3 uniform with name: \"{0}\"", in_key);
		return false;
	}

	SIC_GL_CHECK(glUniform3f(it->second, in_vec3.x, in_vec3.y, in_vec3.z));
	return true;
}

bool sic::OpenGl_program::set_uniform(const char* in_key, const glm::vec4& in_vec4) const
{
	auto it = m_uniforms.find(in_key);

	if (it == m_uniforms.end())
	{
		SIC_LOG_W(g_log_renderer_verbose, "Failed to set vec4 uniform with name: \"{0}\"", in_key);
		return false;
	}

	SIC_GL_CHECK(glUniform4f(it->second, in_vec4.x, in_vec4.y, in_vec4.z, in_vec4.w));
	return true;
}

bool sic::OpenGl_program::set_uniform(const char* in_key, const OpenGl_texture& in_texture) const
{
	return set_uniform_from_bindless_handle(in_key, in_texture.get_bindless_handle());
}

bool sic::OpenGl_program::set_uniform_from_bindless_handle(const char* in_key, GLuint64 in_texture_handle) const
{
	auto it = m_uniforms.find(in_key);

	if (it == m_uniforms.end())
	{
		SIC_LOG_W(g_log_renderer_verbose, "Failed to set bindless texture uniform with name: \"{0}\"", in_key);
		return false;
	}

	SIC_GL_CHECK(glUniformHandleui64ARB(it->second, in_texture_handle));
	return true;
}

bool sic::OpenGl_program::set_uniform_block(const OpenGl_uniform_block& in_block) const
{
	auto it = m_uniform_blocks.find(in_block.get_name());

	if (it == m_uniform_blocks.end())
	{
		SIC_LOG_W(g_log_renderer_verbose, "Failed to set uniform block with name: \"{0}\"", in_block.get_name());
		return false;
	}

	glUniformBlockBinding(m_program_id, it->second, in_block.get_binding_point());
	SIC_GL_CHECK(glBindBufferRange(GL_UNIFORM_BUFFER, in_block.get_binding_point(), in_block.get_block_id(), 0, in_block.get_block_size()));

	return true;
}

std::optional<GLuint> sic::OpenGl_program::get_uniform_location(const char* in_key) const
{
	auto it = m_uniforms.find(in_key);

	if (it == m_uniforms.end())
		return {};

	return it->second;
}

void sic::OpenGl_program::free_resources()
{
	if (m_program_id != 0)
	{
		SIC_GL_CHECK(glDeleteProgram(m_program_id));
		m_program_id = 0;
	}
}

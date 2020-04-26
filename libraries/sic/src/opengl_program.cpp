#pragma once
#include "sic/opengl_program.h"
#include "sic/logger.h"

#include "glm/gtc/type_ptr.hpp"

#include <vector>

sic::OpenGl_program::OpenGl_program(const std::string& in_vertex_shader_name, const std::string& in_vertex_shader_code, const std::string& in_fragment_shader_name, const std::string& in_fragment_shader_code)
{
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
		SIC_LOG(g_log_renderer_verbose, &vertex_shader_error_message[0]);
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
		SIC_LOG_E(g_log_renderer_verbose, &fragment_shader_error_message[0]);
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
		SIC_LOG_E(g_log_renderer_verbose, &program_error_message[0]);
	}

	glDetachShader(m_program_id, vertex_shader_id);
	glDetachShader(m_program_id, fragment_shader_id);

	glDeleteShader(vertex_shader_id);
	glDeleteShader(fragment_shader_id);

	glGetProgramiv(m_program_id, GL_ACTIVE_ATTRIBUTES, &m_attribute_count);
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
	in_other.m_program_id = 0;
}

sic::OpenGl_program& sic::OpenGl_program::operator=(OpenGl_program&& in_other) noexcept
{
	free_resources();

	m_program_id = in_other.m_program_id;
	m_attribute_count = in_other.m_attribute_count;
	in_other.m_program_id = 0;

	return *this;
}

void sic::OpenGl_program::use() const
{
	for (ui32 i = 0; i < m_attribute_count; i++)
		glEnableVertexAttribArray(i);

	for (ui32 i = m_attribute_count; i < sic::max_vertex_attributes; i++)
		glDisableVertexAttribArray(i);

	SIC_GL_CHECK(glUseProgram(m_program_id));
}

bool sic::OpenGl_program::set_uniform(const char* in_key, const glm::mat4& in_matrix) const
{
	GLuint uniform_loc = get_uniform_location(in_key);

	if (uniform_loc == -1)
	{
		SIC_LOG_W(g_log_renderer_verbose, "Failed to bind mat4 uniform with name: \"{0}\"", in_key);
		return false;
	}

	SIC_GL_CHECK(glUniformMatrix4fv(uniform_loc, 1, GL_FALSE, glm::value_ptr(in_matrix)));
	return true;
}

bool sic::OpenGl_program::set_uniform(const char* in_key, const glm::vec3& in_vec3) const
{
	GLuint uniform_loc = get_uniform_location(in_key);

	if (uniform_loc == -1)
	{
		SIC_LOG_W(g_log_renderer_verbose, "Failed to bind vec3 uniform with name: \"{0}\"", in_key);
		return false;
	}

	SIC_GL_CHECK(glUniform3f(uniform_loc, in_vec3.x, in_vec3.y, in_vec3.z));
	return true;
}

bool sic::OpenGl_program::set_uniform(const char* in_key, const glm::vec4& in_vec4) const
{
	GLuint uniform_loc = get_uniform_location(in_key);

	if (uniform_loc == -1)
	{
		SIC_LOG_W(g_log_renderer_verbose, "Failed to bind vec4 uniform with name: \"{0}\"", in_key);
		return false;
	}

	SIC_GL_CHECK(glUniform4f(uniform_loc, in_vec4.x, in_vec4.y, in_vec4.z, in_vec4.w));
	return true;
}

bool sic::OpenGl_program::set_uniform_block(const OpenGl_uniform_block& in_block) const
{
	const GLuint block_index = glGetUniformBlockIndex(m_program_id, in_block.get_name().c_str());

	if (block_index == -1)
	{
		SIC_LOG_W(g_log_renderer_verbose, "Failed to bind uniform block with name: \"{0}\"", in_block.get_name());
		return false;
	}

	glUniformBlockBinding(m_program_id, block_index, in_block.get_binding_point());
	SIC_GL_CHECK(glBindBufferRange(GL_UNIFORM_BUFFER, in_block.get_binding_point(), in_block.get_block_id(), 0, in_block.get_block_size()));

	return true;
}

GLuint sic::OpenGl_program::get_uniform_location(const char* in_key) const
{
	return glGetUniformLocation(m_program_id, in_key);
}

void sic::OpenGl_program::free_resources()
{
	if (m_program_id != 0)
	{
		SIC_GL_CHECK(glDeleteProgram(m_program_id));
		m_program_id = 0;
	}
}

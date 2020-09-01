#pragma once

#include "GL/glew.h"

#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"

#include "sic/core/logger.h"

#define SIC_DEBUG_OPENGL

#ifdef SIC_DEBUG_OPENGL

#define SIC_GL_CHECK(in_function_call)												\
{																					\
	in_function_call;																\
																					\
	GLenum gl_err_value;															\
	while ((gl_err_value = glGetError()) != GL_NO_ERROR)							\
	{																				\
	SIC_LOG_E(g_log_renderer, "OpenGL error: {0}", gluErrorString(gl_err_value));	\
	__debugbreak();																	\
	}																				\
}

#else

#define SIC_GL_CHECK(in_function_call)												\
in_function_call;

#endif
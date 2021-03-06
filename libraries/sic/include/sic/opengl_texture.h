#pragma once
#include "sic/gl_includes.h"
#include "sic/opengl_buffer.h"

#include "sic/core/type_restrictions.h"

#include "glm/vec2.hpp"

namespace sic
{
	enum class OpenGl_texture_format_internal
	{
		invalid = -1,

		depth_component = GL_DEPTH_COMPONENT,
		depth_stencil = GL_DEPTH_STENCIL,

		r = GL_RED,
		rg = GL_RG,
		rgb = GL_RGB,
		rgba = GL_RGBA,

		r8 = GL_R8,
		r8_snorm = GL_R8_SNORM,
		r16 = GL_R16,
		r16_snorm = GL_R16_SNORM,

		rg8 = GL_RG8,
		rg8_snorm = GL_RG8_SNORM,
		rg16 = GL_RG16,
		rg16_snorm = GL_RG16_SNORM,

		r3g3b2 = GL_R3_G3_B2,

		rgb4 = GL_RGB4,
		rgb5 = GL_RGB5,
		rgb8 = GL_RGB8,
		rgb8_snorm = GL_RGB8_SNORM,
		rgb10 = GL_RGB10,
		rgb12 = GL_RGB12,
		rgb16_snorm = GL_RGB16_SNORM,

		rgba32f = GL_RGBA32F
	};

	enum class OpenGl_texture_format
	{
		invalid = -1,

		depth_component = GL_DEPTH_COMPONENT,
		depth_stencil = GL_DEPTH_STENCIL,

		r = GL_RED,
		rg = GL_RG,
		rgb = GL_RGB,
		rgba = GL_RGBA,

		bgr = GL_BGR,
		bgra = GL_BGRA,

		r_integer = GL_RED_INTEGER,
		rg_integer = GL_RG_INTEGER,
		rgb_integer = GL_RGB_INTEGER,
		rgba_integer = GL_RGBA_INTEGER,

		bgr_integer = GL_BGR_INTEGER,
		bgra_integer = GL_BGRA_INTEGER,

		stencil_index = GL_STENCIL_INDEX
	};

	enum class OpenGl_texture_channel_type
	{
		invalid = -1,

		unsigned_byte = GL_UNSIGNED_BYTE,
		byte = GL_BYTE,
		unsigned_short = GL_UNSIGNED_SHORT,
		signed_short = GL_SHORT,
		unsigned_int = GL_UNSIGNED_INT,
		signed_int = GL_INT,
		half_float = GL_HALF_FLOAT,
		whole_float = GL_FLOAT
	};

	enum class OpenGl_texture_mag_filter
	{
		invalid = -1,

		linear = GL_LINEAR,
		nearest = GL_NEAREST
	};

	enum class OpenGl_texture_min_filter
	{
		invalid = -1,

		linear = GL_LINEAR,
		nearest = GL_NEAREST,
		nearest_mipmap_nearest = GL_NEAREST_MIPMAP_NEAREST,
		linear_mipmap_nearest = GL_LINEAR_MIPMAP_NEAREST,
		nearest_mipmap_linear = GL_NEAREST_MIPMAP_LINEAR,
		linear_mipmap_linear = GL_LINEAR_MIPMAP_LINEAR
	};

	enum class OpenGl_texture_wrap_mode
	{
		invalid = -1,

		repeat = GL_REPEAT,
		mirrored_repeat = GL_MIRRORED_REPEAT,
		clamp = GL_CLAMP_TO_EDGE
	};

	struct OpenGl_texture : Noncopyable
	{
		struct Creation_params_texture_base;
		struct Creation_params_2D;
		struct Creation_params_buffer;

		OpenGl_texture(const Creation_params_2D& in_params);
		OpenGl_texture(const Creation_params_buffer& in_params);
		~OpenGl_texture();

		OpenGl_texture(OpenGl_texture&& in_other) noexcept;
		OpenGl_texture& operator=(OpenGl_texture&& in_other)  noexcept;

		void update_data(const glm::ivec2& in_offset, const glm::ivec2& in_dimensions, void* in_data);

		GLuint get_id() const { return m_texture_id; }
		GLuint64 get_bindless_handle() const { return m_bindless_handle; }
		OpenGl_texture_format get_format() const { return m_format; }

	private:
		void free_resources();
		void begin_initialize_texture_params(const Creation_params_texture_base& in_params);
		void end_initialize_texture_params(const Creation_params_texture_base& in_params);

		GLuint m_target_type = 0;
		GLuint m_dimension_format = 0;
		OpenGl_texture_format m_format = OpenGl_texture_format::invalid;
		OpenGl_texture_channel_type m_channel_type = OpenGl_texture_channel_type::invalid;
		GLuint m_texture_id = 0;
		GLuint64 m_bindless_handle = 0;

	public:
		struct Creation_params_texture_base
		{
			friend OpenGl_texture;

			//Sets both format and internal format
			void set_format(OpenGl_texture_format in_format)
			{
				m_format = in_format;
				m_internal_format = static_cast<OpenGl_texture_format_internal>(m_format);
			}

			//Sets internal format individually
			void set_format(OpenGl_texture_format in_format, OpenGl_texture_format_internal in_internal_format)
			{
				m_format = in_format;
				m_internal_format = in_internal_format;
			}

			void set_channel_type(OpenGl_texture_channel_type in_channel_type)
			{
				m_channel_type = in_channel_type;
			}

			void set_filtering(OpenGl_texture_mag_filter in_mag_filter, OpenGl_texture_min_filter in_min_filter)
			{
				m_mag_filter = in_mag_filter;
				m_min_filter = in_min_filter;
			}

			void set_wrap_mode(OpenGl_texture_wrap_mode in_wrap_mode)
			{
				m_wrap_mode = in_wrap_mode;
			}

			void set_data(unsigned char* in_data)
			{
				m_data = in_data;
			}

			void set_debug_name(const std::string& in_name)
			{
				m_debug_name = in_name;
			}

		private:
			std::string m_debug_name;
			OpenGl_texture_format m_format = OpenGl_texture_format::invalid;
			OpenGl_texture_format_internal m_internal_format = OpenGl_texture_format_internal::invalid;
			OpenGl_texture_channel_type m_channel_type = OpenGl_texture_channel_type::invalid;
			OpenGl_texture_mag_filter m_mag_filter = OpenGl_texture_mag_filter::invalid;
			OpenGl_texture_min_filter m_min_filter = OpenGl_texture_min_filter::invalid;
			OpenGl_texture_wrap_mode m_wrap_mode = OpenGl_texture_wrap_mode::repeat;
			unsigned char* m_data = nullptr;
		};

		struct Creation_params_2D : Creation_params_texture_base
		{
			friend OpenGl_texture;

			void set_dimensions(const glm::ivec2& in_dimensions)
			{
				m_dimensions = in_dimensions;
			}

		private:
			glm::ivec2 m_dimensions = { 0, 0 };
		};

		struct Creation_params_buffer
		{
			friend OpenGl_texture;

			//Sets both format and internal format
			void set_format(OpenGl_texture_format in_format)
			{
				m_format = in_format;
				m_internal_format = static_cast<OpenGl_texture_format_internal>(m_format);
			}

			//Sets internal format individually
			void set_format(OpenGl_texture_format in_format, OpenGl_texture_format_internal in_internal_format)
			{
				m_format = in_format;
				m_internal_format = in_internal_format;
			}

			void set_buffer(const OpenGl_buffer& in_buffer)
			{
				m_buffer = &in_buffer;
			}

			void set_debug_name(const std::string& in_name)
			{
				m_debug_name = in_name;
			}

		private:
			std::string m_debug_name;
			OpenGl_texture_format m_format = OpenGl_texture_format::invalid;
			OpenGl_texture_format_internal m_internal_format = OpenGl_texture_format_internal::invalid;
			const OpenGl_buffer* m_buffer = nullptr;
		};
	};
}
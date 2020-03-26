#include "sic/opengl_draw_interface_debug_lines.h"

#include "sic/file_management.h"
#include "sic/opengl_program.h"
#include "sic/opengl_engine_uniform_blocks.h"
#include "sic/opengl_draw_strategies.h"

#include <string>

sic::OpenGl_draw_interface_debug_lines::OpenGl_draw_interface_debug_lines()
{
	m_line_points.resize(max_lines_per_batch * 2);
	m_line_colors.resize(max_lines_per_batch * 2);
}

void sic::OpenGl_draw_interface_debug_lines::begin_frame()
{
	m_line_point_current = m_line_points.data();
	m_line_color_current = m_line_colors.data();
}

void sic::OpenGl_draw_interface_debug_lines::draw_line(const glm::vec3& in_start, const glm::vec3& in_end, const glm::vec4& in_color)
{
	*m_line_point_current = in_start;
	++m_line_point_current;

	*m_line_point_current = in_end;
	++m_line_point_current;

	*m_line_color_current = in_color;
	++m_line_color_current;

	*m_line_color_current = in_color;
	++m_line_color_current;

	if (m_line_point_current - 1 == &m_line_points.back())
		flush();
}

void sic::OpenGl_draw_interface_debug_lines::end_frame()
{
	flush();
}

void sic::OpenGl_draw_interface_debug_lines::flush()
{
	static const std::string simple_line_vertex_shader_path = "content/materials/simple_line.vert";
	static const std::string simple_line_fragment_shader_path = "content/materials/simple_line.frag";
	static OpenGl_program simple_line_program(simple_line_vertex_shader_path, File_management::load_file(simple_line_vertex_shader_path), simple_line_fragment_shader_path, File_management::load_file(simple_line_fragment_shader_path));
	static OpenGl_vertex_buffer_array<OpenGl_vertex_attribute_position3D, OpenGl_vertex_attribute_color> simple_line_vertex_buffer_array;
	static bool has_set_blocks = false;
	if (!has_set_blocks)
	{
		has_set_blocks = true;
		simple_line_program.set_uniform_block(OpenGl_uniform_block_view::get());

		simple_line_vertex_buffer_array.resize<OpenGl_vertex_attribute_position3D, glm::vec3>(m_line_points.size());
		simple_line_vertex_buffer_array.resize<OpenGl_vertex_attribute_color, glm::vec4>(m_line_colors.size());
	}

	simple_line_vertex_buffer_array.set_data_partial<OpenGl_vertex_attribute_position3D>(m_line_points, 0);
	simple_line_vertex_buffer_array.set_data_partial<OpenGl_vertex_attribute_color>(m_line_colors, 0);

	simple_line_program.use();

	simple_line_vertex_buffer_array.bind();
	OpenGl_draw_strategy_line_array::draw(0, m_line_points.size());

	begin_frame();
}

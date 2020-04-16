#include "sic/opengl_draw_interface_debug_lines.h"

#include "sic/file_management.h"
#include "sic/opengl_engine_uniform_blocks.h"
#include "sic/opengl_draw_strategies.h"

#include <string>

sic::OpenGl_draw_interface_debug_lines::OpenGl_draw_interface_debug_lines(const OpenGl_uniform_block_view& in_uniform_block_view) :
	simple_line_program(simple_line_vertex_shader_path, File_management::load_file(simple_line_vertex_shader_path), simple_line_fragment_shader_path, File_management::load_file(simple_line_fragment_shader_path))
{
	m_line_points.resize(max_lines_per_batch * 2);
	m_line_colors.resize(max_lines_per_batch * 2);

	simple_line_program.set_uniform_block(in_uniform_block_view);

	simple_line_vertex_buffer_array.resize<OpenGl_vertex_attribute_position3D, glm::vec3>(static_cast<ui32>(m_line_points.size()));
	simple_line_vertex_buffer_array.resize<OpenGl_vertex_attribute_color, glm::vec4>(static_cast<ui32>(m_line_colors.size()));
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
	simple_line_vertex_buffer_array.set_data_partial<OpenGl_vertex_attribute_position3D>(m_line_points, 0);
	simple_line_vertex_buffer_array.set_data_partial<OpenGl_vertex_attribute_color>(m_line_colors, 0);

	simple_line_program.use();

	simple_line_vertex_buffer_array.bind();
	OpenGl_draw_strategy_line_array::draw(0, static_cast<GLsizei>(m_line_points.size()));

	begin_frame();
}

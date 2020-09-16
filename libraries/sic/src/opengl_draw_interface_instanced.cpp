#include "sic/opengl_draw_interface_instanced.h"
#include "sic/opengl_draw_strategies.h"

void sic::OpenGl_draw_interface_instanced::begin_frame(const OpenGl_vertex_buffer_array_base& in_vba, const OpenGl_buffer& in_index_buffer, Asset_material& in_material)
{
	assert(in_material.m_instance_buffer.size() > 0);

	m_vba = &in_vba;
	m_index_buffer = &in_index_buffer;

	m_material = &in_material;

	const ui32 stride = m_material->m_instance_vec4_stride * uniform_block_alignment_functions::get_alignment<glm::vec4>();
	m_instance_buffer.resize((ui64)in_material.m_max_elements_per_drawcall * stride);
	m_current_instance = m_instance_buffer.data();
}

void sic::OpenGl_draw_interface_instanced::draw_instance(const char* in_buffer_data)
{
	const ui32 stride = m_material->m_instance_vec4_stride * uniform_block_alignment_functions::get_alignment<glm::vec4>();

	memcpy(m_current_instance, in_buffer_data, stride);

	m_current_instance += stride;

	if (m_current_instance >= m_instance_buffer.data() + (m_material->m_max_elements_per_drawcall * stride))
		flush();
}

void sic::OpenGl_draw_interface_instanced::end_frame()
{
	flush();

	m_vba = nullptr;
	m_index_buffer = nullptr;
}

void sic::OpenGl_draw_interface_instanced::flush()
{
	assert(m_vba && m_index_buffer && "Did you forget to call begin_frame?");

	m_material->m_instance_data_buffer.value().set_data_raw(m_instance_buffer.data(), 0, (ui32)m_instance_buffer.size());

	m_vba->bind();
	m_index_buffer->bind();

	m_material->m_program.value().use();

	const ui32 stride = m_material->m_instance_vec4_stride * uniform_block_alignment_functions::get_alignment<glm::vec4>();

	const GLsizei elements_to_draw = static_cast<GLsizei>((m_current_instance - m_instance_buffer.data()) / stride);
	OpenGl_draw_strategy_triangle_element_instanced::draw(m_index_buffer->get_max_elements(), 0, elements_to_draw);

	m_current_instance = m_instance_buffer.data();
}

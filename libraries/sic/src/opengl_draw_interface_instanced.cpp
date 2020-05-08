#include "sic/opengl_draw_interface_instanced.h"

#include "sic/opengl_draw_strategies.h"

void sic::OpenGl_draw_interface_instanced::begin_frame(const Asset_model::Mesh& in_mesh, Asset_material& in_material)
{
	assert(in_material.m_instance_buffer.size() > 0);

	m_mesh = &in_mesh;
	m_material = &in_material;

	m_instance_buffer.resize((ui64)in_material.m_max_elements_per_drawcall * (ui64)in_material.m_instance_stride);
	m_current_instance = m_instance_buffer.data();
}

void sic::OpenGl_draw_interface_instanced::draw_instance(char* in_buffer_data)
{
	memcpy(m_current_instance, in_buffer_data, m_material->m_instance_stride);

	m_current_instance += m_material->m_instance_stride;

	if (m_current_instance == m_instance_buffer.data() + (m_material->m_max_elements_per_drawcall * m_material->m_instance_stride))
		flush();
}

void sic::OpenGl_draw_interface_instanced::end_frame()
{
	flush();

	m_mesh = nullptr;
}

void sic::OpenGl_draw_interface_instanced::flush()
{
	assert(m_mesh && "Did you forget to call begin_frame?");

	m_material->m_instance_data_uniform_block.value().set_data_raw(0, static_cast<ui32>(m_current_instance - m_instance_buffer.data()), m_instance_buffer.data());

	m_mesh->m_vertex_buffer_array.value().bind();
	m_mesh->m_index_buffer.value().bind();

	m_material->m_program.value().use();

	OpenGl_draw_strategy_triangle_element_instanced::draw(m_mesh->m_index_buffer.value().get_max_elements(), 0, static_cast<GLsizei>((m_current_instance - m_instance_buffer.data()) / m_material->m_instance_stride));

	m_current_instance = m_instance_buffer.data();
}

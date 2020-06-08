#include "sic/opengl_draw_interface_instanced.h"

#include "sic/opengl_draw_strategies.h"

void sic::OpenGl_draw_interface_instanced::begin_frame(const Asset_model::Mesh& in_mesh, Asset_material& in_material)
{
	assert(in_material.m_instance_buffer.size() > 0);

	m_mesh = &in_mesh;
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

	m_mesh = nullptr;
}

void sic::OpenGl_draw_interface_instanced::flush()
{
	assert(m_mesh && "Did you forget to call begin_frame?");

	m_material->m_instance_data_texture.value().update_data({0, 0}, glm::ivec2(m_material->m_max_elements_per_drawcall * m_material->m_instance_vec4_stride, 1), m_instance_buffer.data());

	//GLfloat instance_data_texture_vec4_stride = (GLfloat)m_material->m_instance_vec4_stride;
	//GLuint64 tex_handle = m_material->m_instance_data_texture.value().get_bindless_handle();
	//
	//m_material->m_instance_data_uniform_block.value().set_data_raw(0, sizeof(GLfloat), &instance_data_texture_vec4_stride);
	//m_material->m_instance_data_uniform_block.value().set_data_raw(uniform_block_alignment_functions::get_alignment<glm::vec4>(), sizeof(GLuint64), &tex_handle);

	m_mesh->m_vertex_buffer_array.value().bind();
	m_mesh->m_index_buffer.value().bind();

	m_material->m_program.value().use();

	const ui32 stride = m_material->m_instance_vec4_stride * uniform_block_alignment_functions::get_alignment<glm::vec4>();

	const GLsizei elements_to_draw = static_cast<GLsizei>((m_current_instance - m_instance_buffer.data()) / stride);
	OpenGl_draw_strategy_triangle_element_instanced::draw(m_mesh->m_index_buffer.value().get_max_elements(), 0, elements_to_draw);

	m_current_instance = m_instance_buffer.data();
}

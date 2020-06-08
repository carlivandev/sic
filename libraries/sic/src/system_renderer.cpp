#include "sic/system_renderer.h"
#include "sic/component_transform.h"

#include "sic/system_window.h"
#include "sic/component_view.h"
#include "sic/logger.h"
#include "sic/state_render_scene.h"
#include "sic/file_management.h"
#include "sic/system_asset.h"

#include "sic/opengl_vertex_buffer_array.h"
#include "sic/opengl_program.h"
#include "sic/opengl_engine_uniform_blocks.h"
#include "sic/opengl_draw_interface_debug_lines.h"
#include "sic/renderer_shape_draw_functions.h"
#include "sic/opengl_draw_strategies.h"
#include "sic/shader_parser.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/norm.hpp"

#include "stb/stb_image.h"

void sic::System_renderer::on_created(Engine_context in_context)
{
	in_context.register_state<State_render_scene>("State_render_scene");
	in_context.register_state<State_debug_drawing>("State_debug_drawing");
	in_context.register_state<State_renderer_resources>("State_renderer_resources");
	in_context.register_state<State_renderer>("State_renderer");

	in_context.listen<event_created<Level>>
	(
		[](Engine_context& in_out_context, Level& in_out_level)
		{
			if (in_out_level.get_is_root_level())
			{
				State_render_scene& render_scene_state = in_out_context.get_state_checked<State_render_scene>();
				State_debug_drawing& debug_drawing_state = in_out_context.get_state_checked<State_debug_drawing>();

				//only create new render scenes for each root level
				render_scene_state.add_level(in_out_level.m_level_id);
				//only create debug drawer on root levels
				Render_object_id<Render_object_debug_drawer> obj_id = render_scene_state.create_object<Render_object_debug_drawer>(in_out_level.m_level_id, nullptr);
				debug_drawing_state.m_level_id_to_debug_drawer_ids[in_out_level.m_level_id] = obj_id;
			}
		}
	);

	in_context.listen<event_destroyed<Level>>
	(
		[](Engine_context& in_out_context, Level& in_out_level)
		{
			if (in_out_level.get_is_root_level())
			{
				State_render_scene& render_scene_state = in_out_context.get_state_checked<State_render_scene>();
				State_debug_drawing& debug_drawing_state = in_out_context.get_state_checked<State_debug_drawing>();

				auto drawer_id_it = debug_drawing_state.m_level_id_to_debug_drawer_ids.find(in_out_level.m_level_id);
				//only create debug drawer on root levels
				render_scene_state.destroy_object(drawer_id_it->second);

				debug_drawing_state.m_level_id_to_debug_drawer_ids.erase(drawer_id_it);

				//only create new render scenes for each root level
				render_scene_state.remove_level(in_out_level.m_level_id);
			}
		}
	);
}

void sic::System_renderer::on_engine_finalized(Engine_context in_context) const
{
	State_window& window_state = in_context.get_state_checked<State_window>();
	glfwMakeContextCurrent(window_state.m_resource_context);

	State_renderer_resources& resources = in_context.get_state_checked<State_renderer_resources>();

	resources.add_uniform_block<OpenGl_uniform_block_view>();
	resources.add_uniform_block<OpenGl_uniform_block_lights>();

	State_debug_drawing& debug_drawer_state = in_context.get_state_checked<State_debug_drawing>();
	debug_drawer_state.m_draw_interface_debug_lines.emplace(*resources.get_static_uniform_block<OpenGl_uniform_block_view>());
	
	const char* pass_through_vertex_shader_path = "content/engine/materials/pass_through.vert";
	const char* simple_texture_fragment_shader_path = "content/engine/materials/simple_texture.frag";

	resources.m_pass_through_program.emplace
	(
		pass_through_vertex_shader_path,
		Shader_parser::parse_shader(pass_through_vertex_shader_path).value(),
		simple_texture_fragment_shader_path,
		Shader_parser::parse_shader(simple_texture_fragment_shader_path).value()
	);

	resources.m_quad_vertex_buffer_array.emplace();
	resources.m_quad_indexbuffer.emplace(GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW);
	const std::vector<GLfloat> positions =
	{
		-1.0f, -1.0f,
		-1.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, -1.0f
	};

	const std::vector<GLfloat> tex_coords =
	{
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f
	};

	/*
	1 2
	0 3
	*/

	std::vector<unsigned int> indices =
	{
		0, 2, 1,
		0, 3, 2
	};

	resources.m_quad_vertex_buffer_array.value().bind();

	resources.m_quad_vertex_buffer_array.value().set_data<OpenGl_vertex_attribute_position2D>(positions);
	resources.m_quad_vertex_buffer_array.value().set_data<OpenGl_vertex_attribute_texcoord>(tex_coords);

	resources.m_quad_indexbuffer.value().set_data(indices);

	std::vector<unsigned char> white_pixels =
	{
		255, 255, 255, 255,		255, 255, 255, 255,
		255, 255, 255, 255,		255, 255, 255, 255
	};

	//TODO: CONTINUE YES
	resources.m_white_texture.emplace(glm::ivec2(white_pixels.size() / (2 * 4), white_pixels.size() / (2 * 4)), GL_RGBA, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, white_pixels.data());

	State_assetsystem& assetsystem_state = in_context.get_state_checked<State_assetsystem>();
	resources.m_error_material = assetsystem_state.create_asset<Asset_material>("mesh_error_material", "content/engine/materials");
	assetsystem_state.modify_asset<Asset_material>
	(
		resources.m_error_material,
		[](Asset_material* material)
		{
			material->import("content/engine/materials/mesh_error.vert", "content/engine/materials/mesh_error.frag");
		}
	);
}

void sic::System_renderer::on_engine_tick(Engine_context in_context, float in_time_delta) const
{
	in_time_delta;

	State_window& window_state = in_context.get_state_checked<State_window>();
	State_render_scene& scene_state = in_context.get_state_checked<State_render_scene>();
	State_assetsystem& assetsystem_state = in_context.get_state_checked<State_assetsystem>();

	std::scoped_lock asset_modification_lock(assetsystem_state.get_modification_mutex());

	glfwMakeContextCurrent(window_state.m_resource_context);

	do_asset_post_loads(in_context);

	//clear all window backbuffers
	for (auto&& window : scene_state.m_windows.m_objects)
	{
		if (!window.m_context)
			continue;

		glfwMakeContextCurrent(window.m_context);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glfwMakeContextCurrent(window_state.m_resource_context);
		window.m_render_target.value().clear();
	}

	std::unordered_map<Render_object_window*, std::vector<Render_object_view*>> window_to_views_lut;
	
	for (auto&& level_to_scene_it : scene_state.m_level_id_to_scene_lut)
	{
		Update_list<Render_object_view>& views = std::get<Update_list<Render_object_view>>(level_to_scene_it.second);

		for (Render_object_view& view : views.m_objects)
		{
			Render_object_window* window = scene_state.m_windows.find_object(view.m_window_id);
			if (!window)
				continue;

			window_to_views_lut[window].push_back(&view);
		}
	}

	for (auto& window : scene_state.m_windows.m_objects)
	{
		auto& views = window_to_views_lut[&window];

		glfwMakeContextCurrent(window_state.m_resource_context);

		sic::i32 current_window_x, current_window_y;
		glfwGetWindowSize(window.m_context, &current_window_x, &current_window_y);

		if (current_window_x == 0 || current_window_y == 0)
			continue;

		// Enable depth test
		glEnable(GL_DEPTH_TEST);
		// Accept fragment if it closer to the camera than the former one
		glDepthFunc(GL_LESS);

		glEnable(GL_CULL_FACE);

		for (Render_object_view* view : views)
			render_view(in_context, window, *view);
	}
	
	render_views_to_window_backbuffers(window_to_views_lut);

	glfwMakeContextCurrent(window_state.m_resource_context);
	
	for (auto&& scene_it : scene_state.m_level_id_to_scene_lut)
	{
		Update_list<Render_object_debug_drawer>& debug_drawers = std::get<Update_list<Render_object_debug_drawer>>(scene_it.second);

		for (Render_object_debug_drawer& debug_drawer : debug_drawers.m_objects)
			debug_drawer.update_shape_lifetimes(in_time_delta);
	}

	glfwMakeContextCurrent(nullptr);
}

void sic::System_renderer::render_view(Engine_context in_context, const Render_object_window& in_window, Render_object_view& inout_view) const
{
	State_debug_drawing& debug_drawer_state = in_context.get_state_checked<State_debug_drawing>();
	State_renderer_resources& renderer_resources_state = in_context.get_state_checked<State_renderer_resources>();
	State_render_scene& scene_state = in_context.get_state_checked<State_render_scene>();

	auto scene_it = scene_state.m_level_id_to_scene_lut.find(inout_view.m_level_id);

	if (scene_it == scene_state.m_level_id_to_scene_lut.end())
		return;

	const Update_list<Render_object_mesh>& meshes = std::get<Update_list<Render_object_mesh>>(scene_it->second);

	scene_state.m_opaque_drawcalls.clear();
	scene_state.m_translucent_drawcalls.clear();

	scene_state.m_opaque_drawcalls.reserve(meshes.m_objects.size());
	scene_state.m_translucent_drawcalls.reserve(meshes.m_objects.size());

	const glm::vec3 view_location =
	{
		inout_view.m_view_orientation[3][0],
		inout_view.m_view_orientation[3][1],
		inout_view.m_view_orientation[3][2],
	};

	for (const Render_object_mesh& mesh : meshes.m_objects)
	{
		Asset_material* mat = mesh.m_material;

		for (const Material_texture_parameter& texture_param : mat->m_parameters.m_textures)
		{
			if (texture_param.m_texture.is_valid())
			{
				assert(texture_param.m_texture.get_load_state() == Asset_load_state::loaded);
			}
			else
			{
				mat = renderer_resources_state.m_error_material.get_mutable();
				break;
			}
		}

		const GLint mat_attrib_count = mat->m_program.value().get_attribute_count();
		const size_t mesh_attrib_count = mesh.m_mesh->m_vertex_buffer_array.value().get_attribute_count();
		if (mat_attrib_count > mesh_attrib_count)
		{
			SIC_LOG_E
			(
				g_log_renderer, "Failed to add drawcall, material(\"{0})\" expected {1} attributes but mesh only has {2}.",
				mat->m_vertex_shader_path, mat_attrib_count, mesh_attrib_count
			);

			continue;
		}

		char* instance_buffer = mat->m_is_instanced ? mat->m_instance_buffer.data() + mesh.m_instance_data_index : nullptr;

		switch (mat->m_blend_mode)
		{
		case Material_blend_mode::Opaque:
		case Material_blend_mode::Masked:
			scene_state.m_opaque_drawcalls.push_back({ mesh.m_orientation, mesh.m_mesh, mat, &mat->m_parameters, instance_buffer });
			break;

		case Material_blend_mode::Translucent:
		case Material_blend_mode::Additive:
		{
			const glm::vec3 mesh_location =
			{
				mesh.m_orientation[3][0],
				mesh.m_orientation[3][1],
				mesh.m_orientation[3][2],
			};

			scene_state.m_translucent_drawcalls.push_back({ mesh.m_orientation, mesh.m_mesh, mat, &mat->m_parameters, instance_buffer, glm::length2(mesh_location - view_location) });
		}
		break;

		case Material_blend_mode::Invalid:
		default:
			continue;
		}
	}

	sic::i32 current_window_x, current_window_y;
	glfwGetWindowSize(in_window.m_context, &current_window_x, &current_window_y);

	const float view_aspect_ratio = (current_window_x * inout_view.m_viewport_size.x) / (current_window_y * inout_view.m_viewport_size.y);

	glm::ivec2 view_dimensions = inout_view.m_render_target.value().get_dimensions();
	const glm::ivec2 target_dimensions = { current_window_x * inout_view.m_viewport_size.x, current_window_y * inout_view.m_viewport_size.y };

	if (view_dimensions.x != target_dimensions.x ||
		view_dimensions.y != target_dimensions.y)
	{
		inout_view.m_render_target.value().resize(target_dimensions);
		view_dimensions = target_dimensions;
	}

	inout_view.m_render_target.value().bind_as_target(0);
	inout_view.m_render_target.value().clear();

	glViewport
	(
		0,
		0,
		view_dimensions.x,
		view_dimensions.y
	);

	// Set the list of draw buffers.
	GLenum draw_buffers[1] = { GL_COLOR_ATTACHMENT0 };
	SIC_GL_CHECK(glDrawBuffers(1, draw_buffers)); // "1" is the size of DrawBuffers

	const glm::mat4x4 proj_mat = glm::perspective
	(
		glm::radians(inout_view.m_fov),
		view_aspect_ratio,
		inout_view.m_near_plane,
		inout_view.m_far_plane
	);

	const glm::mat4x4 view_mat = glm::inverse(inout_view.m_view_orientation);
	const glm::mat4x4 view_proj_mat = proj_mat * view_mat;

	if (OpenGl_uniform_block_view* view_block = renderer_resources_state.get_static_uniform_block<OpenGl_uniform_block_view>())
	{
		view_block->set_data(0, view_mat);
		view_block->set_data(1, proj_mat);
		view_block->set_data(2, view_proj_mat);
	}

	const glm::vec3 first_light_pos = glm::vec3(100.0f, 100.0f, -300.0f);

	static float cur_val = 0.0f;
	cur_val += 0.0016f;
	const glm::vec3 second_light_pos = glm::vec3(100.0f, (glm::cos(cur_val * 20.0f) * 300.0f), 0.0f);

	std::vector<OpenGl_uniform_block_light_instance> relevant_lights;
	OpenGl_uniform_block_light_instance light;
	light.m_position_and_unused = glm::vec4(first_light_pos, 0.0f);
	light.m_color_and_intensity = { 1.0f, 0.0f, 0.0f, 5000.0f };

	relevant_lights.push_back(light);

	light.m_position_and_unused = glm::vec4(second_light_pos, 0.0f);
	light.m_color_and_intensity = { 0.0f, 1.0f, 0.0f, 5000.0f };

	relevant_lights.push_back(light);

	const ui32 byte_size = static_cast<ui32>(sizeof(OpenGl_uniform_block_light_instance) * relevant_lights.size());

	if (OpenGl_uniform_block_lights* lights_block = renderer_resources_state.get_static_uniform_block<OpenGl_uniform_block_lights>())
	{
		lights_block->set_data(0, static_cast<GLfloat>(relevant_lights.size()));
		lights_block->set_data_raw(1, 0, byte_size, relevant_lights.data());
	}

	set_depth_mode(Depth_mode::read_write);
	set_blend_mode(Material_blend_mode::Opaque);

	render_meshes<Drawcall_mesh, false, false>(scene_state.m_opaque_drawcalls, proj_mat, view_mat, renderer_resources_state);

	debug_drawer_state.m_draw_interface_debug_lines.value().begin_frame();

	Update_list<Render_object_debug_drawer>& debug_drawers = std::get<Update_list<Render_object_debug_drawer>>(scene_it->second);

	for (Render_object_debug_drawer& debug_drawer : debug_drawers.m_objects)
		debug_drawer.draw_shapes(debug_drawer_state.m_draw_interface_debug_lines.value());

	debug_drawer_state.m_draw_interface_debug_lines.value().end_frame();

	set_depth_mode(Depth_mode::read);

	std::sort
	(
		scene_state.m_translucent_drawcalls.begin(), scene_state.m_translucent_drawcalls.end(),
		[](const Drawcall_mesh_translucent& in_a, const Drawcall_mesh_translucent& in_b)
		{
			return in_a.m_distance_to_view_2 < in_b.m_distance_to_view_2;
		}
	);

	render_meshes<Drawcall_mesh_translucent, true, true>(scene_state.m_translucent_drawcalls, proj_mat, view_mat, renderer_resources_state);

	set_depth_mode(Depth_mode::read_write);
	set_blend_mode(Material_blend_mode::Opaque);

	in_window.m_render_target.value().bind_as_target(0);

	SIC_GL_CHECK(glViewport
	(
		static_cast<GLsizei>((current_window_x * inout_view.m_viewport_offset.x)),
		static_cast<GLsizei>(current_window_y * inout_view.m_viewport_offset.y),
		static_cast<GLsizei>((current_window_x * inout_view.m_viewport_size.x)),
		static_cast<GLsizei>(current_window_y * inout_view.m_viewport_size.y)
	));

	renderer_resources_state.m_quad_vertex_buffer_array.value().bind();
	renderer_resources_state.m_quad_indexbuffer.value().bind();

	renderer_resources_state.m_pass_through_program.value().use();

	renderer_resources_state.m_pass_through_program.value().set_uniform("uniform_texture", inout_view.m_render_target.value().get_texture());

	OpenGl_draw_strategy_triangle_element::draw(static_cast<GLsizei>(renderer_resources_state.m_quad_indexbuffer.value().get_max_elements()), 0);
}

void sic::System_renderer::render_mesh(const Drawcall_mesh& in_dc, const glm::mat4& in_mvp) const
{
	in_dc.m_mesh->m_vertex_buffer_array.value().bind();
	in_dc.m_mesh->m_index_buffer.value().bind();

	auto& program = in_dc.m_material->m_program.value();
	program.use();

	if (program.get_uniform_location("MVP"))
		program.set_uniform("MVP", in_mvp);

	if (program.get_uniform_location("model_matrix"))
		program.set_uniform("model_matrix", in_dc.m_orientation);

	for (auto& texture_param : in_dc.m_parameters->m_textures)
	{
		if (!program.set_uniform(texture_param.m_name.c_str(), texture_param.m_texture.get()->m_texture.value()))
			SIC_LOG_E(g_log_renderer_verbose, "Texture parameter: {0}", texture_param.m_name.c_str());
	}

	OpenGl_draw_strategy_triangle_element::draw(static_cast<GLsizei>(in_dc.m_mesh->m_index_buffer.value().get_max_elements()), 0);
}

void sic::System_renderer::render_views_to_window_backbuffers(const std::unordered_map<sic::Render_object_window*, std::vector<sic::Render_object_view*>>& in_window_to_view_lut) const
{
	for (auto& window_to_views_it : in_window_to_view_lut)
	{
		glfwMakeContextCurrent(window_to_views_it.first->m_context);
		window_to_views_it.first->draw_to_backbuffer();

		glfwSwapBuffers(window_to_views_it.first->m_context);
	}
}

void sic::System_renderer::do_asset_post_loads(Engine_context in_context) const
{
	State_assetsystem& assetsystem_state = in_context.get_state_checked<State_assetsystem>();
	State_renderer_resources& renderer_resources_state = in_context.get_state_checked<State_renderer_resources>();

	assetsystem_state.do_post_load<Asset_texture>
	(
		[](Asset_texture& in_texture)
		{
			//do opengl load
			post_load_texture(in_texture);
		}
	);

	assetsystem_state.do_post_load<Asset_material>
	(
		[&renderer_resources_state](Asset_material& in_material)
		{
			//do opengl load
			post_load_material(renderer_resources_state, in_material);
		}
	);

	assetsystem_state.do_post_load<Asset_model>
	(
		[](Asset_model& in_model)
		{
			//do opengl load
			for (auto&& mesh : in_model.m_meshes)
				post_load_mesh(mesh);
		}
	);

	assetsystem_state.do_pre_unload<Asset_texture>
	(
		[](Asset_texture& in_texture)
		{
			in_texture.m_texture.reset();

			if (!in_texture.m_free_texture_data_after_setup)
				in_texture.m_texture_data.reset();
		}
	);

	assetsystem_state.do_pre_unload<Asset_material>
	(
		[](Asset_material& in_material)
		{
			in_material.m_program.reset();
		}
	);

	assetsystem_state.do_pre_unload<Asset_model>
	(
		[](Asset_model& in_model)
		{
			for (auto&& mesh : in_model.m_meshes)
			{
				mesh.m_vertex_buffer_array.reset();
				mesh.m_index_buffer.reset();
			}
		}
	);
}

void sic::System_renderer::post_load_texture(Asset_texture& out_texture)
{
	i32 gl_texture_format = 0;

	switch (out_texture.m_format)
	{
	case Texture_format::gray:
		gl_texture_format = GL_R;
		break;
	case Texture_format::gray_a:
		gl_texture_format = GL_RG;
		break;
	case Texture_format::rgb:
		gl_texture_format = GL_RGB;
		break;
	case Texture_format::rgb_a:
		gl_texture_format = GL_RGBA;
		break;
	default:
		break;
	}

	out_texture.m_texture.emplace(glm::ivec2(out_texture.m_width, out_texture.m_height), gl_texture_format, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, out_texture.m_texture_data.get());

	if (out_texture.m_free_texture_data_after_setup)
		out_texture.m_texture_data.reset();
}

void sic::System_renderer::post_load_material(const State_renderer_resources& in_resource_state, Asset_material& out_material)
{
	if (out_material.m_vertex_shader_code.empty() || out_material.m_fragment_shader_code.empty())
		return;

	//if we have a parent we will share its rendering resources, only parameters will be unique
	if (out_material.m_parent.is_valid())
		return;

	out_material.m_program.emplace(out_material.m_vertex_shader_path, out_material.m_vertex_shader_code, out_material.m_fragment_shader_path, out_material.m_fragment_shader_code);

	if (!out_material.m_program.value().get_is_valid())
	{
		out_material.m_program.reset();
		return;
	}

	if (out_material.m_is_instanced)
	{
		out_material.m_instance_data_texture.emplace
		(
			glm::ivec2(out_material.m_max_elements_per_drawcall * out_material.m_instance_vec4_stride, 1),
			GL_RGBA32F,
			GL_RGBA,
			GL_FLOAT,
			GL_LINEAR,
			GL_LINEAR_MIPMAP_LINEAR,
			nullptr
		);

		out_material.m_program.value().set_uniform_block(out_material.m_instance_data_uniform_block.emplace(out_material.m_instance_block_name.c_str(), GL_DYNAMIC_DRAW, uniform_block_alignment_functions::get_alignment<glm::vec4>() + uniform_block_alignment_functions::get_alignment<GLuint64>()));

		float instance_data_texture_vec4_stride = out_material.m_instance_vec4_stride;
		GLuint64 tex_handle = out_material.m_instance_data_texture.value().get_bindless_handle();

		out_material.m_instance_data_uniform_block.value().set_data_raw(0, sizeof(GLfloat), &instance_data_texture_vec4_stride);
		out_material.m_instance_data_uniform_block.value().set_data_raw(uniform_block_alignment_functions::get_alignment<glm::vec4>(), sizeof(GLuint64), &tex_handle);
	}

	for (auto&& uniform_block_mapping : out_material.m_program.value().get_uniform_blocks())
		if (const OpenGl_uniform_block* block = in_resource_state.get_uniform_block(uniform_block_mapping.first))
			out_material.m_program.value().set_uniform_block(*block);
}

void sic::System_renderer::post_load_mesh(Asset_model::Mesh& inout_mesh)
{
	auto& vertex_buffer_array = inout_mesh.m_vertex_buffer_array.emplace();
	vertex_buffer_array.bind();

	vertex_buffer_array.set_data<OpenGl_vertex_attribute_position3D>(inout_mesh.m_positions);
	vertex_buffer_array.set_data<OpenGl_vertex_attribute_normal>(inout_mesh.m_normals);
	vertex_buffer_array.set_data<OpenGl_vertex_attribute_texcoord>(inout_mesh.m_texcoords);
	vertex_buffer_array.set_data<OpenGl_vertex_attribute_tangent>(inout_mesh.m_tangents);
	vertex_buffer_array.set_data<OpenGl_vertex_attribute_bitangent>(inout_mesh.m_bitangents);

	auto& index_buffer = inout_mesh.m_index_buffer.emplace(GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW);
	index_buffer.set_data(inout_mesh.m_indices);
}

void sic::System_renderer::set_depth_mode(Depth_mode in_to_set) const
{
	switch (in_to_set)
	{
	case sic::Depth_mode::disabled:
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		break;

	case sic::Depth_mode::read_write:
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		break;

	case sic::Depth_mode::read:
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		break;

	case sic::Depth_mode::write:
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		break;

	default:
		assert(false);
		break;
	}
}

void sic::System_renderer::set_blend_mode(Material_blend_mode in_to_set) const
{
	switch (in_to_set)
	{
	case sic::Material_blend_mode::Opaque:
	case sic::Material_blend_mode::Masked:
		glDisable(GL_BLEND);
		break;

	case sic::Material_blend_mode::Translucent:
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		break;

	case sic::Material_blend_mode::Additive:
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		break;

	case sic::Material_blend_mode::Invalid:
		assert(false);
		break;

	default:
		assert(false);
		break;
	}
}
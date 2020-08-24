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

#include "msdfgen-master/msdfgen.h"
#include "msdfgen-master/msdfgen-ext.h"

void sic::System_renderer::on_created(Engine_context in_context)
{
 	msdfgen::FreetypeHandle* ft = msdfgen::initializeFreetype();
	if (ft)
	{
		msdfgen::FontHandle* font = msdfgen::loadFont(ft, "C:\\Windows\\Fonts\\arialbd.ttf");
		if (font)
		{
			msdfgen::Shape shape;
			if (msdfgen::loadGlyph(shape, font, 'A'))
			{
				shape.normalize();
				//                      max. angle
				msdfgen::edgeColoringSimple(shape, 3.0);
				//           image width, height
				msdfgen::Bitmap<float, 3> msdf(32, 32);
				//                     range, scale, translation
				msdfgen::generateMSDF(msdf, shape, 4.0, 1.0, msdfgen::Vector2(4.0, 4.0));
				msdfgen::savePng(msdf, "output.png");
				
				std::vector<byte> pixels(3 * msdf.width() * msdf.height());
				std::vector<byte>::iterator it = pixels.begin();
				for (int y = msdf.height() - 1; y >= 0; --y)
					for (int x = 0; x < msdf.width(); ++x)
					{
						*it++ = msdfgen::pixelFloatToByte(msdf(x, y)[0]);
						*it++ = msdfgen::pixelFloatToByte(msdf(x, y)[1]);
						*it++ = msdfgen::pixelFloatToByte(msdf(x, y)[2]);
					}
			}
			msdfgen::destroyFont(font);
		}
		msdfgen::deinitializeFreetype(ft);
	}


	in_context.register_state<State_render_scene>("State_render_scene");
	in_context.register_state<State_debug_drawing>("State_debug_drawing");
	in_context.register_state<State_renderer_resources>("State_renderer_resources");
	in_context.register_state<State_renderer>("State_renderer");

	in_context.listen<Event_created<Scene>>
	(
		[](Engine_context& in_out_context, Scene& in_out_level)
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

	in_context.listen<event_destroyed<Scene>>
	(
		[](Engine_context& in_out_context, Scene& in_out_level)
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

	State_assetsystem& assetsystem_state = in_context.get_state_checked<State_assetsystem>();

	assetsystem_state.register_type<Asset_texture>();
	assetsystem_state.register_type<Asset_render_target>();
	assetsystem_state.register_type<Asset_material>();
	assetsystem_state.register_type<Asset_model>();
}

void sic::System_renderer::on_engine_finalized(Engine_context in_context) const
{
	State_window& window_state = in_context.get_state_checked<State_window>();
	glfwMakeContextCurrent(window_state.m_resource_context);

	State_renderer_resources& resources = in_context.get_state_checked<State_renderer_resources>();

	resources.add_uniform_block<OpenGl_uniform_block_view>();
	resources.add_uniform_block<OpenGl_uniform_block_lights>();
	resources.add_uniform_block<OpenGl_uniform_block_instancing>();

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

	{
		resources.m_quad_vertex_buffer_array.emplace();
		resources.m_quad_indexbuffer.emplace(OpenGl_buffer::Creation_params(OpenGl_buffer_target::element_array, OpenGl_buffer_usage::static_draw));
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
	}

	std::vector<unsigned char> white_pixels =
	{
		255, 255, 255, 255,		255, 255, 255, 255,
		255, 255, 255, 255,		255, 255, 255, 255
	};

	//TODO: CONTINUE YES
	//resources.m_white_texture.emplace(glm::ivec2(white_pixels.size() / (2 * 4), white_pixels.size() / (2 * 4)), GL_RGBA, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, white_pixels.data());

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
		window.m_ui_render_target.value().clear();
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

		for (Render_object_view* view : views)
			render_view(in_context, window, *view);

		render_ui(in_context, window);
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
	const OpenGl_render_target* view_render_target = nullptr;

	if (inout_view.m_render_target.is_valid() && inout_view.m_render_target.get_load_state() == Asset_load_state::loaded)
		view_render_target = &(inout_view.m_render_target.get()->m_render_target.value());
	else
		view_render_target = &(in_window.m_render_target.value()); //if no explicit render target is defined, use "fast-path" and render directly to window

	if (!view_render_target)
		return;

	State_debug_drawing& debug_drawer_state = in_context.get_state_checked<State_debug_drawing>();
	State_renderer_resources& renderer_resources_state = in_context.get_state_checked<State_renderer_resources>();
	State_render_scene& scene_state = in_context.get_state_checked<State_render_scene>();

	auto scene_it = scene_state.m_level_id_to_scene_lut.find(inout_view.m_level_id);

	if (scene_it == scene_state.m_level_id_to_scene_lut.end())
		return;

	set_depth_mode(Depth_mode::read_write);
	set_blend_mode(Material_blend_mode::Opaque);

	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);

	glEnable(GL_CULL_FACE);

	glm::ivec2 view_dimensions = view_render_target->get_dimensions();

	const float view_aspect_ratio = (view_dimensions.x * inout_view.m_viewport_size.x) / (view_dimensions.y * inout_view.m_viewport_size.y);

	const glm::ivec2 target_dimensions = { view_dimensions.x * inout_view.m_viewport_size.x, view_dimensions.y * inout_view.m_viewport_size.y };

	view_render_target->bind_as_target(0);
	view_render_target->clear();

	SIC_GL_CHECK(glViewport
	(
		static_cast<GLsizei>((view_dimensions.x * inout_view.m_viewport_offset.x)),
		static_cast<GLsizei>(view_dimensions.y * inout_view.m_viewport_offset.y),
		static_cast<GLsizei>((view_dimensions.x * inout_view.m_viewport_size.x)),
		static_cast<GLsizei>(view_dimensions.y * inout_view.m_viewport_size.y)
	));

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

	const glm::vec3 view_location =
	{
		inout_view.m_view_orientation[3][0],
		inout_view.m_view_orientation[3][1],
		inout_view.m_view_orientation[3][2],
	};

	if (OpenGl_uniform_block_view* view_block = renderer_resources_state.get_static_uniform_block<OpenGl_uniform_block_view>())
	{
		view_block->set_data(0, view_mat);
		view_block->set_data(1, proj_mat);
		view_block->set_data(2, view_proj_mat);
	}

	{
		Render_all_3d_objects_data render_data;
		render_data.m_meshes = &std::get<Update_list<Render_object_mesh>>(scene_it->second);
		render_data.m_debug_drawer = &std::get<Update_list<Render_object_debug_drawer>>(scene_it->second);
		render_data.m_scene_state = &scene_state;
		render_data.m_renderer_resources_state = &renderer_resources_state;
		render_data.m_debug_drawer_state = &debug_drawer_state;
		render_data.m_view_mat = view_mat;
		render_data.m_proj_mat = proj_mat;
		render_data.m_view_location = view_location;

		render_all_3d_objects(render_data);
		//insert post-processing here
	}

	set_depth_mode(Depth_mode::read_write);
	set_blend_mode(Material_blend_mode::Opaque);
}

void sic::System_renderer::render_ui(Engine_context in_context, const Render_object_window& in_window) const
{
	State_renderer_resources& renderer_resources_state = in_context.get_state_checked<State_renderer_resources>();
	State_render_scene& scene_state = in_context.get_state_checked<State_render_scene>();

	auto& ui_elements = scene_state.m_ui_elements;

	scene_state.m_ui_drawcalls.clear();
	scene_state.m_ui_drawcalls.reserve(ui_elements.m_objects.size());

	for (const Render_object_ui& element : ui_elements.m_objects)
	{
		if (scene_state.m_windows.find_object(element.m_window_id) != &in_window)
			continue;

		Asset_material* child_mat = element.m_material;
		Asset_material* mat = element.m_material->m_outermost_parent.is_valid() ? element.m_material->m_outermost_parent.get_mutable() : element.m_material;
		assert(mat);

		const GLint mat_attrib_count = mat->m_program.value().get_attribute_count();
		const size_t mesh_attrib_count = renderer_resources_state.m_quad_vertex_buffer_array.value().get_attribute_count();
		if (mat_attrib_count > mesh_attrib_count)
		{
			SIC_LOG_E
			(
				g_log_renderer, "Failed to add drawcall, material(\"{0})\" expected {1} attributes but mesh only has {2}.",
				mat->m_vertex_shader_path, mat_attrib_count, mesh_attrib_count
			);

			continue;
		}

		byte* instance_buffer = mat->m_instance_buffer.data() + element.m_instance_data_index;

		switch (mat->m_blend_mode)
		{
		case Material_blend_mode::Opaque:
		case Material_blend_mode::Masked:
			SIC_LOG_E(g_log_renderer, "Failed to add UI drawcall, material(\"{0})\" has an invalid blendmode, only Translucent/Additive is supported.", mat->get_header().m_name);
			continue;
			break;

		case Material_blend_mode::Translucent:
		case Material_blend_mode::Additive:
		{
			scene_state.m_ui_drawcalls.push_back(Drawcall_ui_element(element.m_lefttop, element.m_rightbottom, mat, instance_buffer, element.m_sort_priority, element.m_custom_sort_priority));
		}
		break;

		case Material_blend_mode::Invalid:
		default:
			continue;
		}
	}

	if (scene_state.m_ui_drawcalls.empty())
		return;

	in_window.m_ui_render_target.value().bind_as_target(0);
	in_window.m_ui_render_target.value().clear();

	glViewport
	(
		0,
		0,
		in_window.m_render_target->get_dimensions().x,
		in_window.m_render_target->get_dimensions().y
	);

	set_depth_mode(Depth_mode::disabled);
	renderer_resources_state.m_quad_vertex_buffer_array.value().bind();
	renderer_resources_state.m_quad_indexbuffer.value().bind();
	const GLsizei idx_buffer_max_elements_count = static_cast<GLsizei>(renderer_resources_state.m_quad_indexbuffer.value().get_max_elements());

	std::sort
	(
		scene_state.m_ui_drawcalls.begin(), scene_state.m_ui_drawcalls.end(),
		[](const Drawcall_ui_element& in_a, const Drawcall_ui_element& in_b)
		{
			if (in_a.m_sort_priority < in_b.m_sort_priority)
				return true;
			else if (in_a.m_sort_priority > in_b.m_sort_priority)
				return false;

			return in_a.m_custom_sort_priority < in_b.m_sort_priority;
		}
	);

	auto instanced_begin = sort_instanced(scene_state.m_ui_drawcalls);

	const char* lefttop_rightbottom_packed_name = "lefttop_rightbottom_packed";

	struct Local
	{
		static glm::vec2 round_to_pixel_density(const glm::vec2& in_vec, const glm::ivec2& in_pixel_density)
		{
			glm::vec2 ret_val = { in_vec.x * in_pixel_density.x, in_vec.y * in_pixel_density.y };
			ret_val = glm::round(ret_val);
			ret_val.x /= in_pixel_density.x;
			ret_val.y /= in_pixel_density.y;

			return ret_val;
		}
	};

	for (auto it = scene_state.m_ui_drawcalls.begin(); it != instanced_begin; ++it)
	{
		set_blend_mode(it->m_material->m_blend_mode);

		const auto& program = it->m_material->m_program.value();
		program.use();

		const glm::vec2 top_left = Local::round_to_pixel_density(it->m_topleft, in_window.m_render_target->get_dimensions());
		const glm::vec2 bottom_right = Local::round_to_pixel_density(it->m_bottomright, in_window.m_render_target->get_dimensions());

		const glm::vec4 lefttop_rightbottom_packed = { top_left.x, top_left.y, bottom_right.x, bottom_right.y };

		if (program.get_uniform_location(lefttop_rightbottom_packed_name))
			program.set_uniform(lefttop_rightbottom_packed_name, lefttop_rightbottom_packed);

		apply_parameters(*it->m_material, it->m_instance_data, program);

		OpenGl_draw_strategy_triangle_element::draw(idx_buffer_max_elements_count, 0);
	}

	auto&& chunks = gather_instancing_chunks(instanced_begin, scene_state.m_ui_drawcalls.end());

	for (auto&& chunk : chunks)
	{
		auto lefttop_rightbottom_packed_loc_it = chunk.m_begin->m_material->m_instance_data_name_to_offset_lut.find(lefttop_rightbottom_packed_name);

		if (lefttop_rightbottom_packed_loc_it != chunk.m_begin->m_material->m_instance_data_name_to_offset_lut.end())
		{
			GLuint lefttop_rightbottom_packed_loc = lefttop_rightbottom_packed_loc_it->second;
			for (auto it = chunk.m_begin; it != chunk.m_end; ++it)
			{
				const glm::vec2 top_left = Local::round_to_pixel_density(it->m_topleft, in_window.m_render_target->get_dimensions());
				const glm::vec2 bottom_right = Local::round_to_pixel_density(it->m_bottomright, in_window.m_render_target->get_dimensions());

				const glm::vec4 lefttop_rightbottom_packed = { top_left.x, top_left.y, bottom_right.x, bottom_right.y };

				memcpy(it->m_instance_data + lefttop_rightbottom_packed_loc, &lefttop_rightbottom_packed, uniform_block_alignment_functions::get_alignment<glm::vec4>());
			}
		}

		render_instancing_chunk(chunk, renderer_resources_state.m_quad_vertex_buffer_array.value(), renderer_resources_state.m_quad_indexbuffer.value(), renderer_resources_state);
	}

	//insert post-processing here

	in_window.m_render_target.value().bind_as_target(0);

	glViewport
	(
		0,
		0,
		in_window.m_render_target->get_dimensions().x,
		in_window.m_render_target->get_dimensions().y
	);

	renderer_resources_state.m_quad_vertex_buffer_array.value().bind();
	renderer_resources_state.m_quad_indexbuffer.value().bind();

	renderer_resources_state.m_pass_through_program.value().use();

	set_depth_mode(Depth_mode::disabled);
	set_blend_mode(Material_blend_mode::Translucent);

	renderer_resources_state.m_pass_through_program.value().set_uniform("uniform_texture", in_window.m_ui_render_target.value().get_texture());

	OpenGl_draw_strategy_triangle_element::draw(static_cast<GLsizei>(renderer_resources_state.m_quad_indexbuffer.value().get_max_elements()), 0);
}

void sic::System_renderer::render_all_3d_objects(Render_all_3d_objects_data in_data) const
{
	const Update_list<Render_object_mesh>& meshes = *in_data.m_meshes;
	Update_list<Render_object_debug_drawer>& debug_drawers = *in_data.m_debug_drawer;
	State_render_scene& scene_state = *in_data.m_scene_state;
	State_renderer_resources& renderer_resources_state = *in_data.m_renderer_resources_state;
	State_debug_drawing& debug_drawer_state = *in_data.m_debug_drawer_state;

	const glm::mat4x4 view_mat = in_data.m_view_mat;
	const glm::mat4x4 proj_mat = in_data.m_proj_mat;
	const glm::vec3 view_location = in_data.m_view_location;

	scene_state.m_opaque_drawcalls.clear();
	scene_state.m_translucent_drawcalls.clear();

	scene_state.m_opaque_drawcalls.reserve(meshes.m_objects.size());
	scene_state.m_translucent_drawcalls.reserve(meshes.m_objects.size());

	for (const Render_object_mesh& mesh : meshes.m_objects)
	{
		Asset_material* child_mat = mesh.m_material;
		Asset_material* mat = mesh.m_material->m_outermost_parent.is_valid() ? mesh.m_material->m_outermost_parent.get_mutable() : mesh.m_material;
		assert(mat);

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

		char* instance_buffer = mat->m_instance_buffer.data() + mesh.m_instance_data_index;

		switch (mat->m_blend_mode)
		{
		case Material_blend_mode::Opaque:
		case Material_blend_mode::Masked:
			scene_state.m_opaque_drawcalls.push_back(Drawcall_mesh(mesh.m_orientation, mesh.m_mesh, mat, instance_buffer));
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

			scene_state.m_translucent_drawcalls.push_back(Drawcall_mesh_translucent(mesh.m_orientation, mesh.m_mesh, mat, instance_buffer, glm::length2(mesh_location - view_location)));
		}
		break;

		case Material_blend_mode::Invalid:
		default:
			continue;
		}
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

	{
		set_depth_mode(Depth_mode::read_write);
		set_blend_mode(Material_blend_mode::Opaque);

		auto instanced_begin = sort_instanced(scene_state.m_opaque_drawcalls);

		render_meshes(scene_state.m_opaque_drawcalls.begin(), instanced_begin, proj_mat, view_mat);
 		render_meshes_instanced(instanced_begin, scene_state.m_opaque_drawcalls.end(), proj_mat, view_mat, renderer_resources_state);
	}

	debug_drawer_state.m_draw_interface_debug_lines.value().begin_frame();

	for (Render_object_debug_drawer& debug_drawer : debug_drawers.m_objects)
		debug_drawer.draw_shapes(debug_drawer_state.m_draw_interface_debug_lines.value());

	debug_drawer_state.m_draw_interface_debug_lines.value().end_frame();

	{
		set_depth_mode(Depth_mode::read);

		std::sort
		(
			scene_state.m_translucent_drawcalls.begin(), scene_state.m_translucent_drawcalls.end(),
			[](const Drawcall_mesh_translucent& in_a, const Drawcall_mesh_translucent& in_b)
			{
				return in_a.m_distance_to_view_2 < in_b.m_distance_to_view_2;
			}
		);

		auto instanced_begin = sort_instanced(scene_state.m_translucent_drawcalls);

		render_meshes(scene_state.m_translucent_drawcalls.begin(), instanced_begin, proj_mat, view_mat);
		render_meshes_instanced(instanced_begin, scene_state.m_translucent_drawcalls.end(), proj_mat, view_mat, renderer_resources_state);
	}
}

void sic::System_renderer::render_views_to_window_backbuffers(const std::unordered_map<sic::Render_object_window*, std::vector<sic::Render_object_view*>>& in_window_to_view_lut) const
{
	set_depth_mode(Depth_mode::read_write);
	set_blend_mode(Material_blend_mode::Opaque);

	for (auto& window_to_views_it : in_window_to_view_lut)
	{
		glfwMakeContextCurrent(window_to_views_it.first->m_context);
		window_to_views_it.first->draw_to_backbuffer();

		glfwSwapBuffers(window_to_views_it.first->m_context);
	}
}

void sic::System_renderer::apply_parameters(const Asset_material& in_material, const byte* in_instance_data, const OpenGl_program& in_program) const
{
	for (auto& param : in_material.m_parameters.get_textures())
	{
		auto offset = in_material.m_instance_data_name_to_offset_lut.find(param.m_name);
		assert(offset != in_material.m_instance_data_name_to_offset_lut.end());

		const GLuint64 texture_handle = *reinterpret_cast<const GLuint64*>(in_instance_data + offset->second);

		if (!in_program.set_uniform_from_bindless_handle(param.m_name.c_str(), texture_handle))
			SIC_LOG_E(g_log_renderer_verbose, "Texture parameter: {0}", param.m_name.c_str());
	}

	for (auto& param : in_material.m_parameters.get_vec4s())
	{
		auto offset = in_material.m_instance_data_name_to_offset_lut.find(param.m_name);
		assert(offset != in_material.m_instance_data_name_to_offset_lut.end());

		const glm::vec4 vec4_value = *reinterpret_cast<const glm::vec4*>(in_instance_data + offset->second);

		if (!in_program.set_uniform(param.m_name.c_str(), vec4_value))
			SIC_LOG_E(g_log_renderer_verbose, "Vec4 parameter: {0}", param.m_name.c_str());
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

	assetsystem_state.do_post_load<Asset_render_target>
	(
		[](Asset_render_target& in_rt)
		{
			//do opengl load
			post_load_render_target(in_rt);
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

	assetsystem_state.do_pre_unload<Asset_render_target>
	(
		[](Asset_render_target& in_rt)
		{
			in_rt.m_render_target.reset();
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
	OpenGl_texture_format gl_texture_format = OpenGl_texture_format::invalid;

	switch (out_texture.m_format)
	{
	case Texture_format::gray:
		gl_texture_format = OpenGl_texture_format::r;
		break;
	case Texture_format::gray_a:
		gl_texture_format = OpenGl_texture_format::rg;
		break;
	case Texture_format::rgb:
		gl_texture_format = OpenGl_texture_format::rgb;
		break;
	case Texture_format::rgb_a:
		gl_texture_format = OpenGl_texture_format::rgba;
		break;
	default:
		break;
	}

	OpenGl_texture::Creation_params_2D params;
	params.set_dimensions(glm::ivec2(out_texture.m_width, out_texture.m_height));
	params.set_format(gl_texture_format);
	params.set_filtering(OpenGl_texture_mag_filter::linear, OpenGl_texture_min_filter::linear_mipmap_linear);
	params.set_channel_type(OpenGl_texture_channel_type::unsigned_byte);
	params.set_data(out_texture.m_texture_data.get());
	params.set_debug_name(out_texture.get_header().m_name);

	out_texture.m_texture.emplace(params);
	out_texture.m_bindless_handle = out_texture.m_texture.value().get_bindless_handle();

	if (out_texture.m_free_texture_data_after_setup)
		out_texture.m_texture_data.reset();
}

void sic::System_renderer::post_load_render_target(Asset_render_target& out_rt)
{
	OpenGl_texture_format gl_texture_format = OpenGl_texture_format::invalid;

	switch (out_rt.m_format)
	{
	case Texture_format::gray:
		gl_texture_format = OpenGl_texture_format::r;
		break;
	case Texture_format::gray_a:
		gl_texture_format = OpenGl_texture_format::rg;
		break;
	case Texture_format::rgb:
		gl_texture_format = OpenGl_texture_format::rgb;
		break;
	case Texture_format::rgb_a:
		gl_texture_format = OpenGl_texture_format::rgba;
		break;
	default:
		break;
	}

	OpenGl_render_target::Creation_params params;
	params.m_dimensions = { out_rt.m_width, out_rt.m_height };
	params.m_texture_format = gl_texture_format;
	params.m_depth_test = out_rt.m_depth_test;

	out_rt.m_render_target.emplace(params);

	out_rt.m_bindless_handle = out_rt.m_render_target->get_texture().get_bindless_handle();
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
		out_material.m_instance_data_buffer.emplace(OpenGl_buffer::Creation_params(OpenGl_buffer_target::texture, OpenGl_buffer_usage::dynamic_draw)).resize<glm::vec4>(out_material.m_max_elements_per_drawcall * out_material.m_instance_vec4_stride);

		OpenGl_texture::Creation_params_buffer params;
		params.set_format(OpenGl_texture_format::rgba, OpenGl_texture_format_internal::rgba32f);
		params.set_buffer(out_material.m_instance_data_buffer.value());
		
		params.set_debug_name(fmt::format("{0} instancing data", out_material.get_header().m_name));

		out_material.m_instance_data_texture.emplace(params);
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

	auto& index_buffer = inout_mesh.m_index_buffer.emplace(OpenGl_buffer::Creation_params(OpenGl_buffer_target::element_array, OpenGl_buffer_usage::static_draw));
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
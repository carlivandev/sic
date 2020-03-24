#include "sic/system_renderer.h"
#include "sic/component_transform.h"

#include "sic/asset_types.h"
#include "sic/system_window.h"
#include "sic/component_view.h"
#include "sic/logger.h"
#include "sic/state_render_scene.h"
#include "sic/file_management.h"
#include "sic/system_asset.h"
#include "sic/opengl_vertex_buffer_array.h"
#include "sic/opengl_program.h"
#include "sic/opengl_engine_uniform_blocks.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "stb/stb_image.h"

namespace sic_private
{
	using namespace sic;

	void init_texture(Asset_texture& out_texture)
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

	void init_material(Asset_material& out_material)
	{
		out_material.m_program.emplace(out_material.m_vertex_shader_path, out_material.m_vertex_shader_code, out_material.m_fragment_shader_path, out_material.m_fragment_shader_code);

		out_material.m_program.value().set_uniform_block(OpenGl_uniform_block_view::get());
		out_material.m_program.value().set_uniform_block(OpenGl_uniform_block_lights::get());
	}

	void init_mesh(Asset_model::Mesh& inout_mesh)
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

	void draw_mesh(const Asset_model::Mesh& in_mesh, const Asset_material& in_material, const glm::mat4& in_mvp, const glm::mat4& in_model_matrix)
	{
		auto& program = in_material.m_program.value();
		program.use();

		program.set_uniform("MVP", in_mvp);
		program.set_uniform("model_matrix", in_model_matrix);

		ui32 texture_param_idx = 0;

		for (auto& texture_param : in_material.m_texture_parameters)
		{
			//in future we want to render an error texture instead
			if (!texture_param.m_texture.is_valid())
				continue;

			const i32 uniform_loc = in_material.m_program.value().get_uniform_location(texture_param.m_name.c_str());

			//error handle this
			if (uniform_loc == -1)
			{
				SIC_LOG_E(g_log_renderer_verbose,
					"Texture parameter: {0}, not found in material with shaders: {1} and {2}",
					texture_param.m_name.c_str(), in_material.m_vertex_shader_path.c_str(), in_material.m_fragment_shader_path.c_str());
				continue;
			}

			texture_param.m_texture.get()->m_texture.value().bind(uniform_loc, texture_param_idx);
			++texture_param_idx;
		}

		// draw mesh
		in_mesh.m_vertex_buffer_array.value().bind();
		in_mesh.m_index_buffer.value().bind();
		glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(in_mesh.m_index_buffer.value().size()), GL_UNSIGNED_INT, 0);

		glActiveTexture(GL_TEXTURE0);
	}

	void draw_lines(const std::vector<glm::vec3>& in_lines, const glm::vec4& in_color);

	void draw_line(const glm::vec3& in_start, const glm::vec3& in_end, const glm::vec4& in_color)
	{
		draw_lines({ in_start, in_end }, in_color);
	}

	void draw_cube(const glm::vec3& in_center, const glm::vec3& in_half_size, const glm::quat& in_rotation, const glm::vec4& in_color)
	{
		std::vector<glm::vec3> lines;

		glm::vec3 start = in_rotation * (glm::vec3(in_half_size.x, in_half_size.y, in_half_size.z));
		glm::vec3 end = in_rotation * (glm::vec3(in_half_size.x, -in_half_size.y, in_half_size.z));
		lines.push_back(in_center + start);
		lines.push_back(in_center + end);

		start = in_rotation * (glm::vec3(in_half_size.x, -in_half_size.y, in_half_size.z));
		end = in_rotation * (glm::vec3(-in_half_size.x, -in_half_size.y, in_half_size.z));
		lines.push_back(in_center + start);
		lines.push_back(in_center + end);

		start = in_rotation * (glm::vec3(-in_half_size.x, -in_half_size.y, in_half_size.z));
		end = in_rotation * (glm::vec3(-in_half_size.x, in_half_size.y, in_half_size.z));
		lines.push_back(in_center + start);
		lines.push_back(in_center + end);

		start = in_rotation * (glm::vec3(-in_half_size.x, in_half_size.y, in_half_size.z));
		end = in_rotation * (glm::vec3(in_half_size.x, in_half_size.y, in_half_size.z));
		lines.push_back(in_center + start);
		lines.push_back(in_center + end);

		start = in_rotation * (glm::vec3(in_half_size.x, in_half_size.y, -in_half_size.z));
		end = in_rotation * (glm::vec3(in_half_size.x, -in_half_size.y, -in_half_size.z));
		lines.push_back(in_center + start);
		lines.push_back(in_center + end);

		start = in_rotation * (glm::vec3(in_half_size.x, -in_half_size.y, -in_half_size.z));
		end = in_rotation * (glm::vec3(-in_half_size.x, -in_half_size.y, -in_half_size.z));
		lines.push_back(in_center + start);
		lines.push_back(in_center + end);

		start = in_rotation * (glm::vec3(-in_half_size.x, -in_half_size.y, -in_half_size.z));
		end = in_rotation * (glm::vec3(-in_half_size.x, in_half_size.y, -in_half_size.z));
		lines.push_back(in_center + start);
		lines.push_back(in_center + end);

		start = in_rotation * (glm::vec3(-in_half_size.x, in_half_size.y, -in_half_size.z));
		end = in_rotation * (glm::vec3(in_half_size.x, in_half_size.y, -in_half_size.z));
		lines.push_back(in_center + start);
		lines.push_back(in_center + end);

		start = in_rotation * (glm::vec3(in_half_size.x, in_half_size.y, in_half_size.z));
		end = in_rotation * (glm::vec3(in_half_size.x, in_half_size.y, -in_half_size.z));
		lines.push_back(in_center + start);
		lines.push_back(in_center + end);

		start = in_rotation * (glm::vec3(in_half_size.x, -in_half_size.y, in_half_size.z));
		end = in_rotation * (glm::vec3(in_half_size.x, -in_half_size.y, -in_half_size.z));
		lines.push_back(in_center + start);
		lines.push_back(in_center + end);

		start = in_rotation * (glm::vec3(-in_half_size.x, -in_half_size.y, in_half_size.z));
		end = in_rotation * (glm::vec3(-in_half_size.x, -in_half_size.y, -in_half_size.z));
		lines.push_back(in_center + start);
		lines.push_back(in_center + end);

		start = in_rotation * (glm::vec3(-in_half_size.x, in_half_size.y, in_half_size.z));
		end = in_rotation * (glm::vec3(-in_half_size.x, in_half_size.y, -in_half_size.z));
		lines.push_back(in_center + start);
		lines.push_back(in_center + end);
		
		draw_lines(lines, in_color);
	}

	void draw_sphere(const glm::vec3& in_center, float in_radius, i32 in_segments, const glm::vec4& in_color)
	{
		std::vector<glm::vec3> lines;
		in_segments = (glm::max)(in_segments, 4);

		glm::vec3 vertex_0, vertex_1, vertex_2, vertex_3;
		const float angle_inc = 2.f * sic::constants::pi / float(in_segments);
		i32 num_segments_y = in_segments;
		float latitude = angle_inc;
		i32 num_segments_x;
		float longitude;
		float sin_y0 = 0.0f, cos_y0 = 1.0f, sin_y1, cos_y1;
		float sin_x, cos_x;

		lines.reserve(num_segments_y * in_segments * 2);

		while (num_segments_y--)
		{
			sin_y1 = glm::sin(latitude);
			cos_y1 = glm::cos(latitude);

			vertex_0 = glm::vec3(sin_y0, cos_y0, 0.0f) * in_radius + in_center;
			vertex_2 = glm::vec3(sin_y1, cos_y1, 0.0f) * in_radius + in_center;
			longitude = angle_inc;

			num_segments_x = in_segments;
			while (num_segments_x--)
			{
				sin_x = glm::sin(longitude);
				cos_x = glm::cos(longitude);

				vertex_1 = glm::vec3((cos_x * sin_y0), cos_y0, (sin_x * sin_y0)) * in_radius + in_center;
				vertex_3 = glm::vec3((cos_x * sin_y1), cos_y1, (sin_x * sin_y1)) * in_radius + in_center;

				lines.push_back(vertex_0);
				lines.push_back(vertex_1);
				lines.push_back(vertex_0);
				lines.push_back(vertex_2);

				vertex_0 = vertex_1;
				vertex_2 = vertex_3;
				longitude += angle_inc;
			}
			sin_y0 = sin_y1;
			cos_y0 = cos_y1;
			latitude += angle_inc;
		}

		draw_lines(lines, in_color);
	}

	//angles are in radians
	void draw_cone(const glm::vec3& in_origin, const glm::vec3& in_direction, float in_length, float in_angle_width, float in_angle_height, i32 in_num_sides, const glm::vec4& in_color)
	{
		in_num_sides = (glm::max)(in_num_sides, 4);

		const float angle0 = glm::clamp(in_angle_height, 0.001f, (float)(sic::constants::pi - 0.001f));
		const float angle1 = glm::clamp(in_angle_width, 0.001f, (float)(sic::constants::pi - 0.001f));

		const float sin_x_2 = glm::sin(0.5f * angle0);
		const float sin_y_2 = glm::sin(0.5f * angle1);

		const float sin_sq_x_2 = sin_x_2 * sin_x_2;
		const float sin_sq_y_2 = sin_y_2 * sin_y_2;

		const float tan_x_2 = glm::tan(0.5f * angle0);
		const float tan_y_2 = glm::tan(0.5f * angle1);

		std::vector<glm::vec3> cone_vertices;
		cone_vertices.resize(in_num_sides);

		for (i32 i = 0; i < in_num_sides; i++)
		{
			const float fraction = (float)i / (float)(in_num_sides);
			const float thi = 2.f * sic::constants::pi * fraction;
			const float phi = std::atan2(glm::sin(thi) * sin_y_2, glm::cos(thi) * sin_x_2);
			const float sin_phi = glm::sin(phi);
			const float cos_phi = glm::cos(phi);
			const float sin_sq_phi = sin_phi * sin_phi;
			const float cos_sq_phi = cos_phi * cos_phi;

			const float r_sq = sin_sq_x_2 * sin_sq_y_2 / (sin_sq_x_2 * sin_sq_phi + sin_sq_y_2 * cos_sq_phi);
			const float r = glm::sqrt(r_sq);
			const float sqr = glm::sqrt(1 - r_sq);
			const float alpha = r * cos_phi;
			const float beta = r * sin_phi;

			cone_vertices[i].x = 2 * sqr * alpha;
			cone_vertices[i].y = 2 * sqr * beta;
			cone_vertices[i].z = -(1 - 2 * r_sq);
		}

		sic::Transform transform;
		transform.set_translation(in_origin);
		transform.look_at(in_direction, sic::Transform::up);

		const glm::mat4x4 cone_to_world = glm::scale(transform.get_matrix(), glm::vec3(in_length));

		std::vector<glm::vec3> lines;
		lines.reserve(in_num_sides);

		glm::vec3 current_point, previous_point, first_point;
		for (i32 i = 0; i < in_num_sides; i++)
		{
			current_point = cone_to_world * glm::vec4(cone_vertices[i], 1.0f);
			lines.push_back(in_origin);
			lines.push_back(current_point);

			// previous_point must be defined to draw junctions
			if (i > 0)
			{
				lines.push_back(previous_point);
				lines.push_back(current_point);
			}
			else
			{
				first_point = current_point;
			}

			previous_point = current_point;
		}
		lines.push_back(current_point);
		lines.push_back(first_point);

		draw_lines(lines, in_color);
	}

	void draw_half_circle(const glm::vec3& in_base, const glm::vec3& in_z_axis, const glm::vec3& in_x_axis, const glm::vec4& in_color, float in_radius, i32 in_num_sides)
	{
		float angle_delta = 2.0f * (float)sic::constants::pi / ((float)in_num_sides);
		glm::vec3 last_vertex = in_base + in_z_axis * in_radius;

		for (i32 side_index = 0; side_index < (in_num_sides / 2); side_index++)
		{
			const glm::vec3 vertex = in_base + (in_z_axis * glm::cos(angle_delta * (side_index + 1)) + in_x_axis * glm::sin(angle_delta * (side_index + 1))) * in_radius;
			draw_line(last_vertex, vertex, in_color);
			last_vertex = vertex;
		}
	}

	void DrawCircle(const glm::vec3& in_base, const glm::vec3& in_z_axis, const glm::vec3& in_x_axis, const glm::vec4& in_color, float in_radius, i32 in_num_sides)
	{
		const float	angle_delta = 2.0f * sic::constants::pi / in_num_sides;
		glm::vec3 last_vertex = in_base + in_z_axis * in_radius;

		for (i32 side_index = 0; side_index < in_num_sides; side_index++)
		{
			const glm::vec3 vertex = in_base + (in_z_axis * glm::cos(angle_delta * (side_index + 1)) + in_x_axis * glm::sin(angle_delta * (side_index + 1))) * in_radius;
			draw_line(last_vertex, vertex, in_color);
			last_vertex = vertex;
		}
	}

	void draw_capsule(const glm::vec3& in_center, float in_half_height, float in_radius, const glm::quat& in_rotation, const glm::vec4& in_color)
	{
		const i32 draw_collision_sides = 32;

		Transform transform;
		transform.set_rotation(in_rotation);
		
		const glm::vec3 x_axis = transform.get_right();
		const glm::vec3 y_axis = transform.get_up();
		const glm::vec3 z_axis = transform.get_forward();

		// Draw top and bottom circles
		const float half_axis = (glm::max)(in_half_height - in_radius, 1.f);
		const glm::vec3 top_end = in_center + half_axis * y_axis;
		const glm::vec3 bottom_end = in_center - half_axis * y_axis;

		DrawCircle(top_end, z_axis, x_axis, in_color, in_radius, draw_collision_sides);
		DrawCircle(bottom_end, z_axis, x_axis, in_color, in_radius, draw_collision_sides);

		// Draw domed caps
		draw_half_circle(top_end, x_axis, y_axis, in_color, in_radius, draw_collision_sides);
		draw_half_circle(top_end, z_axis, y_axis, in_color, in_radius, draw_collision_sides);

		const glm::vec3 negative_y_axis = -y_axis;

		draw_half_circle(bottom_end, x_axis, negative_y_axis, in_color, in_radius, draw_collision_sides);
		draw_half_circle(bottom_end, z_axis, negative_y_axis, in_color, in_radius, draw_collision_sides);

		// Draw connected lines
		draw_line(top_end + in_radius * z_axis, bottom_end + in_radius * z_axis, in_color);
		draw_line(top_end - in_radius * z_axis, bottom_end - in_radius * z_axis, in_color);
		draw_line(top_end + in_radius * x_axis, bottom_end + in_radius * x_axis, in_color);
		draw_line(top_end - in_radius * x_axis, bottom_end - in_radius * x_axis, in_color);
	}

	void draw_frustum(const glm::mat4x4& in_frustum_to_world, const glm::vec4& in_color)
	{
		glm::vec3 vertices[2][2][2];
		for (ui32 z = 0; z < 2; z++)
		{
			for (ui32 y = 0; y < 2; y++)
			{
				for (ui32 x = 0; x < 2; x++)
				{
					const glm::vec4 unprojected_vertex = in_frustum_to_world *
					(
						glm::vec4
						(
							(x ? -1.0f : 1.0f),
							(y ? -1.0f : 1.0f),
							(z ? 0.0f : 1.0f),
							1.0f
						)
					);
					vertices[x][y][z] = glm::vec3(unprojected_vertex) / unprojected_vertex.w;
				}
			}
		}

		draw_line(vertices[0][0][0], vertices[0][0][1], in_color);
		draw_line(vertices[1][0][0], vertices[1][0][1], in_color);
		draw_line(vertices[0][1][0], vertices[0][1][1], in_color);
		draw_line(vertices[1][1][0], vertices[1][1][1], in_color);

		draw_line(vertices[0][0][0], vertices[0][1][0], in_color);
		draw_line(vertices[1][0][0], vertices[1][1][0], in_color);
		draw_line(vertices[0][0][1], vertices[0][1][1], in_color);
		draw_line(vertices[1][0][1], vertices[1][1][1], in_color);

		draw_line(vertices[0][0][0], vertices[1][0][0], in_color);
		draw_line(vertices[0][1][0], vertices[1][1][0], in_color);
		draw_line(vertices[0][0][1], vertices[1][0][1], in_color);
		draw_line(vertices[0][1][1], vertices[1][1][1], in_color);
	}

	void draw_lines(const std::vector<glm::vec3>& in_lines, const glm::vec4& in_color)
	{
		static const std::string debug_shape_vertex_shader_path = "content/materials/debug_shape.vert";
		static const std::string debug_shape_fragment_shader_path = "content/materials/debug_shape.frag";
		static OpenGl_program debug_shape_program(debug_shape_vertex_shader_path, File_management::load_file(debug_shape_vertex_shader_path), debug_shape_fragment_shader_path, File_management::load_file(debug_shape_fragment_shader_path));
		static OpenGl_vertex_buffer_array<OpenGl_vertex_attribute_position3D> debug_shape_vertex_buffer_array;
		static bool has_set_blocks = false;
		if (!has_set_blocks)
		{
			has_set_blocks = true;
			debug_shape_program.set_uniform_block(OpenGl_uniform_block_view::get());
		}

		debug_shape_vertex_buffer_array.set_data<OpenGl_vertex_attribute_position3D>(in_lines);

		debug_shape_program.use();
		debug_shape_program.set_uniform("color", in_color);

		debug_shape_vertex_buffer_array.bind();
		glDrawArrays(GL_LINES, 0, in_lines.size());
	}
}

void sic::System_renderer::on_created(Engine_context&& in_context)
{
	in_context.register_state<State_render_scene>("state_render_scene");
}

void sic::System_renderer::on_engine_tick(Engine_context&& in_context, float in_time_delta) const
{
	in_time_delta;

	State_assetsystem* assetsystem_state = in_context.get_state<State_assetsystem>();

	if (!assetsystem_state)
		return;

	State_window* window_state = in_context.get_state<State_window>();

	glfwMakeContextCurrent(window_state->m_main_window->get<Component_window>().m_window);
	
	assetsystem_state->do_post_load<Asset_texture>
	(
		[](Asset_texture& in_texture)
		{
			//do opengl load
			sic_private::init_texture(in_texture);
		}
	);

	assetsystem_state->do_post_load<Asset_material>
	(
		[](Asset_material& in_material)
		{
			//do opengl load
			sic_private::init_material(in_material);
		}
	);

	assetsystem_state->do_post_load<Asset_model>
	(
		[](Asset_model& in_model)
		{
			//do opengl load
			for (auto&& mesh : in_model.m_meshes)
				sic_private::init_mesh(mesh);
		}
	);

	assetsystem_state->do_pre_unload<Asset_texture>
	(
		[](Asset_texture& in_texture)
		{
			in_texture.m_texture.reset();

			if (!in_texture.m_free_texture_data_after_setup)
				in_texture.m_texture_data.reset();
		}
	);

	assetsystem_state->do_pre_unload<Asset_material>
	(
		[](Asset_material& in_material)
		{
			in_material.m_program.reset();
		}
	);

	assetsystem_state->do_pre_unload<Asset_model>
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

	State_render_scene* scene_state = in_context.get_state<State_render_scene>();

	if (!scene_state)
		return;

	std::vector<Asset_ref<Asset_texture>> textures_to_load;
	std::vector<Asset_ref<Asset_material>> materials_to_load;
	std::vector<Asset_ref<Asset_model>> models_to_load;

	std::unordered_map<GLFWwindow*, std::vector<Render_object_view*>> window_to_views_lut;

	for (Render_object_view& view : scene_state->m_views.m_objects)
	{
		if (!view.m_window_render_on)
			continue;

		window_to_views_lut[view.m_window_render_on].push_back(&view);
	}

	for (auto& window_to_views_it : window_to_views_lut)
	{
		glfwMakeContextCurrent(window_to_views_it.first);

		sic::i32 current_window_x, current_window_y;
		glfwGetWindowSize(window_to_views_it.first, &current_window_x, &current_window_y);

		if (current_window_x == 0 || current_window_y == 0)
			continue;

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		const float aspect_ratio = static_cast<float>(current_window_x) / static_cast<float>(current_window_y);

		for (Render_object_view* view : window_to_views_it.second)
		{
			auto scene_it = scene_state->m_level_id_to_scene_lut.find(view->m_level_id);

			if (scene_it == scene_state->m_level_id_to_scene_lut.end())
				continue;

			view->m_render_target.value().bind_as_target(0);
			view->m_render_target.value().clear();
			const glm::ivec2& view_dimensions = view->m_render_target.value().get_dimensions();

			glViewport
			(
				0,
				0,
				view_dimensions.x,
				view_dimensions.y
			);

			// Set the list of draw buffers.
			GLenum draw_buffers[1] = { GL_COLOR_ATTACHMENT0 };
			glDrawBuffers(1, draw_buffers); // "1" is the size of DrawBuffers

			const glm::mat4x4 proj_mat = glm::perspective
			(
				glm::radians(view->m_fov),
				aspect_ratio,
				view->m_near_plane,
				view->m_far_plane
			);

			const glm::mat4x4 view_mat = glm::inverse(view->m_view_orientation);
			const glm::mat4x4 view_proj_mat = proj_mat * view_mat;

			OpenGl_uniform_block_view::get().set_data(0, view_mat);
			OpenGl_uniform_block_view::get().set_data(1, proj_mat);
			OpenGl_uniform_block_view::get().set_data(2, view_proj_mat);

			const glm::vec3 first_light_pos = glm::vec3(10.0f, 10.0f, -30.0f);

			static float cur_val = 0.0f;
			cur_val += in_time_delta;
			const glm::vec3 second_light_pos = glm::vec3(10.0f, (glm::cos(cur_val * 2.0f) * 30.0f), 0.0f);

			std::vector<OpenGl_uniform_block_light_instance> relevant_lights;
			OpenGl_uniform_block_light_instance light;
			light.m_position_and_unused = glm::vec4(first_light_pos, 0.0f);
			light.m_color_and_intensity = { 1.0f, 0.0f, 0.0f, 100.0f };

			relevant_lights.push_back(light);

			light.m_position_and_unused = glm::vec4(second_light_pos, 0.0f);
			light.m_color_and_intensity = { 0.0f, 1.0f, 0.0f, 50.0f };

			relevant_lights.push_back(light);

			sic_private::draw_line(second_light_pos, first_light_pos, light.m_color_and_intensity);
			sic_private::draw_sphere(light.m_position_and_unused, 32.0f, 16, light.m_color_and_intensity);

			sic_private::draw_cube(light.m_position_and_unused, glm::vec3(32.0f, 32.0f, 32.0f), glm::quat(glm::vec3(0.0f, 0.0f, cur_val)), light.m_color_and_intensity);

			sic_private::draw_cone(first_light_pos, glm::normalize(second_light_pos - first_light_pos), 32.0f, glm::radians(45.0f), glm::radians(45.0f), 16, light.m_color_and_intensity);

			sic_private::draw_capsule(first_light_pos, 20.0f, 10.0f, glm::quat(glm::vec3(0.0f, 0.0f, cur_val)), glm::vec4(1.0f, 0.0f, 1.0f, 1.0f));

			for (Render_object_view* debug_draw_view : window_to_views_it.second)
			{
				if (view == debug_draw_view)
					continue;

				const glm::mat4x4 debug_draw_proj_mat = glm::perspective
				(
					glm::radians(debug_draw_view->m_fov),
					aspect_ratio,
					debug_draw_view->m_near_plane,
					debug_draw_view->m_far_plane
				);
				
				const glm::mat4x4 debug_draw_view_frustum_to_world = debug_draw_view->m_view_orientation * glm::inverse(debug_draw_proj_mat);
				sic_private::draw_frustum(debug_draw_view_frustum_to_world, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
			}
			
			const ui32 byte_size = sizeof(OpenGl_uniform_block_light_instance) * relevant_lights.size();

			OpenGl_uniform_block_lights::get().set_data(0, static_cast<GLfloat>(relevant_lights.size()));
			OpenGl_uniform_block_lights::get().set_data_raw(1, 0, byte_size, relevant_lights.data());

			const Update_list<Render_object_model>& models = std::get<Update_list<Render_object_model>>(scene_it->second);
			for (const Render_object_model& model : models.m_objects)
			{
				if (!model.m_model.is_valid())
					continue;

				if (model.m_model.get_load_state() == Asset_load_state::loaded)
				{
					Asset_model* model_asset = model.m_model.get();

					const glm::mat4 mvp = proj_mat * view_mat * model.m_orientation;

					const ui64 mesh_count = model_asset->m_meshes.size();

					for (i32 mesh_idx = 0; mesh_idx < mesh_count; mesh_idx++)
					{
						auto& mesh = model_asset->m_meshes[mesh_idx];

						auto material_override_it = model.m_material_overrides.find(mesh.m_material_slot);

						const Asset_ref<Asset_material>& mat_to_draw = material_override_it != model.m_material_overrides.end() ? material_override_it->second : model_asset->get_material(mesh_idx);
						if (!mat_to_draw.is_valid())
							continue;

						if (mat_to_draw.get_load_state() == Asset_load_state::loaded)
						{
							bool all_texture_loaded = true;

							for (Asset_material::Texture_parameter& texture_param : mat_to_draw.get()->m_texture_parameters)
							{
								if (texture_param.m_texture.is_valid())
								{
									if (texture_param.m_texture.get_load_state() == Asset_load_state::not_loaded)
										textures_to_load.push_back(texture_param.m_texture);

									if (texture_param.m_texture.get_load_state() != Asset_load_state::loaded)
										all_texture_loaded = false;
								}
							}

							if (all_texture_loaded)
								sic_private::draw_mesh(mesh, *mat_to_draw.get(), mvp, model.m_orientation);
						}
						else if (mat_to_draw.get_load_state() == Asset_load_state::not_loaded)
						{
							materials_to_load.push_back(mat_to_draw);
						}
					}
				}
				else if (model.m_model.get_load_state() == Asset_load_state::not_loaded)
					models_to_load.push_back(model.m_model);
			}

			static bool one_shot = false;

			const std::string quad_vertex_shader_path = "content/materials/pass_through.vert";
			const std::string quad_fragment_shader_path = "content/materials/simple_texture.frag";
			static OpenGl_program quad_program(quad_vertex_shader_path, File_management::load_file(quad_vertex_shader_path), quad_fragment_shader_path, File_management::load_file(quad_fragment_shader_path));

			static OpenGl_vertex_buffer_array
				<
				OpenGl_vertex_attribute_position2D,
				OpenGl_vertex_attribute_texcoord
				> quad_vertex_buffer_array;

			static OpenGl_buffer quad_indexbuffer(GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW);

			if (!one_shot)
			{
				

				one_shot = true;

				const std::vector<GLfloat> positions = {
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
				std::vector<unsigned int> indices = {
					0, 2, 1,
					0, 3, 2
				};

				quad_vertex_buffer_array.bind();

				quad_vertex_buffer_array.set_data<OpenGl_vertex_attribute_position2D>(positions);
				quad_vertex_buffer_array.set_data<OpenGl_vertex_attribute_texcoord>(tex_coords);

				quad_indexbuffer.set_data(indices);

				//OpenGl_uniform_block_test::get().set_data(0, glm::vec3(1.0f, 0.0f, 0.0f));
				//OpenGl_uniform_block_test::get().set_data(1, glm::vec3(0.0f, 0.0f, 1.0f));
				//
				//quad_program.set_uniform_block(OpenGl_uniform_block_test::get());
			}

			// render to  backbuffer
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			glViewport
			(
				static_cast<GLsizei>((current_window_x * view->m_viewport_offset.x)),
				static_cast<GLsizei>(current_window_y * view->m_viewport_offset.y),
				static_cast<GLsizei>((current_window_x * view->m_viewport_size.x)),
				static_cast<GLsizei>(current_window_y * view->m_viewport_size.y)
			);

			quad_program.use();

			GLuint tex_loc_id = quad_program.get_uniform_location("uniform_texture");

			view->m_render_target.value().m_texture.bind(tex_loc_id, 0);

			quad_vertex_buffer_array.bind();
			quad_indexbuffer.bind();

			glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(quad_indexbuffer.size()), GL_UNSIGNED_INT, 0);
		}

		glfwSwapBuffers(window_to_views_it.first);
	}

	glfwMakeContextCurrent(nullptr);

	assetsystem_state->load_batch(in_context, std::move(textures_to_load));
	assetsystem_state->load_batch(in_context, std::move(materials_to_load));
	assetsystem_state->load_batch(in_context, std::move(models_to_load));
}
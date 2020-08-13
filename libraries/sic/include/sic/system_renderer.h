#pragma once
#include "sic/system.h"

#include "sic/engine.h"
#include "sic/opengl_draw_interface_debug_lines.h"
#include "sic/opengl_engine_uniform_blocks.h"
#include "sic/asset_types.h"
#include "sic/double_buffer.h"
#include "sic/state_render_scene.h"
#include "sic/opengl_texture.h"
#include "sic/opengl_draw_interface_instanced.h"

#include "glm/mat4x4.hpp"

#include <unordered_map>
#include <optional>
#include <vector>

namespace sic
{
	struct Render_object_window;
	struct Render_object_view;
	struct Render_object_debug_drawer;

	enum class Depth_mode
	{
		disabled,
		read_write,
		read,
		write
	};

	template<typename t>
	struct Render_object_id;

	struct State_debug_drawing : State
	{
		std::unordered_map<i32, Render_object_id<Render_object_debug_drawer>> m_level_id_to_debug_drawer_ids;
		std::optional<OpenGl_draw_interface_debug_lines> m_draw_interface_debug_lines;
	};

	struct State_renderer_resources : State
	{
		template <typename T_block_type>
		void add_uniform_block()
		{
			T_block_type* new_block = new T_block_type();

			std::unique_ptr<OpenGl_uniform_block>& block_ptr = m_uniform_blocks[new_block->get_name()];
			assert(block_ptr.get() == nullptr && "Block with same name already added!");

			block_ptr = std::unique_ptr<T_block_type>(new_block);
		}

		template <typename T_block_type>
		T_block_type* get_static_uniform_block()
		{
			auto it = m_uniform_blocks.find(T_block_type::get_static_name());

			if (it == m_uniform_blocks.end())
				return nullptr;

			return reinterpret_cast<T_block_type*>(it->second.get());
		}

		template <typename T_block_type>
		const T_block_type* get_static_uniform_block() const
		{
			auto it = m_uniform_blocks.find(T_block_type::get_static_name());

			if (it == m_uniform_blocks.end())
				return nullptr;

			return reinterpret_cast<const T_block_type*>(it->second.get());
		}

		OpenGl_uniform_block* get_uniform_block(const std::string& in_name)
		{
			auto it = m_uniform_blocks.find(in_name);

			if (it == m_uniform_blocks.end())
				return nullptr;

			return it->second.get();
		}

		const OpenGl_uniform_block* get_uniform_block(const std::string& in_name) const
		{
			auto it = m_uniform_blocks.find(in_name);

			if (it == m_uniform_blocks.end())
				return nullptr;

			return it->second.get();
		}

		OpenGl_draw_interface_instanced m_draw_interface_instanced;
		std::optional<OpenGl_program> m_pass_through_program;
		std::optional<OpenGl_vertex_buffer_array
			<
			OpenGl_vertex_attribute_position2D,
			OpenGl_vertex_attribute_texcoord
			>> m_quad_vertex_buffer_array;

		std::optional<OpenGl_buffer> m_quad_indexbuffer;
		std::optional<OpenGl_texture> m_white_texture;
		Asset_ref<Asset_material> m_error_material;

	private:
		std::unordered_map<std::string, std::unique_ptr<OpenGl_uniform_block>> m_uniform_blocks;
	};

	struct State_renderer : State
	{
		friend struct System_renderer_state_swapper;

		using Synchronous_update_signature = std::function<void(Engine_context& inout_context)>;

		void push_synchronous_renderer_update(Synchronous_update_signature in_update)
		{
			m_synchronous_renderer_updates.write
			(
				[in_update](std::vector<std::function<void(Engine_context&)>>& in_write)
				{
					in_write.push_back(in_update);
				}
			);
		}

	private:
		Double_buffer<std::vector<Synchronous_update_signature>> m_synchronous_renderer_updates;
	};

	struct System_renderer : System
	{
		virtual void on_created(Engine_context in_context) override;
		virtual void on_engine_finalized(Engine_context in_context) const override;
		virtual void on_engine_tick(Engine_context in_context, float in_time_delta) const override;

	private:
		void render_view(Engine_context in_context, const Render_object_window& in_window, Render_object_view& inout_view) const;

		template <typename T_drawcall_type, bool T_custom_blend_mode, bool T_stable_partition>
		void render_meshes(std::vector<T_drawcall_type>& inout_drawcalls, const glm::mat4& in_projection_matrix, const glm::mat4& in_view_matrix, State_renderer_resources& inout_renderer_resources_state) const;

		void render_mesh(const Drawcall_mesh& in_dc, const glm::mat4& in_mvp) const;
		void render_views_to_window_backbuffers(const std::unordered_map<sic::Render_object_window*, std::vector<sic::Render_object_view*>>& in_window_to_view_lut) const;

		void do_asset_post_loads(Engine_context in_context) const;

	public:
		static void post_load_texture(Asset_texture& out_texture);
		static void post_load_material(const State_renderer_resources& in_resource_state, Asset_material& out_material);
		static void post_load_mesh(Asset_model::Mesh& inout_mesh);

	private:
		void set_depth_mode(Depth_mode in_to_set) const;
		void set_blend_mode(Material_blend_mode in_to_set) const;
	};
	template<typename T_drawcall_type, bool T_custom_blend_mode, bool T_stable_partition>
	inline void System_renderer::render_meshes(std::vector<T_drawcall_type>& inout_drawcalls, const glm::mat4& in_projection_matrix, const glm::mat4& in_view_matrix, State_renderer_resources& inout_renderer_resources_state) const
	{
		auto instanced_begin = T_stable_partition ?
		std::stable_partition
		(
			inout_drawcalls.begin(), inout_drawcalls.end(),
			[](const T_drawcall_type& in_a)
			{
				return !in_a.m_material->m_is_instanced;
			}
		)
		:
		std::partition
		(
			inout_drawcalls.begin(), inout_drawcalls.end(),
			[](const T_drawcall_type& in_a)
			{
				return !in_a.m_material->m_is_instanced;
			}
		);

		for (auto it = inout_drawcalls.begin(); it != instanced_begin; ++it)
		{
			if constexpr (T_custom_blend_mode)
				set_blend_mode(it->m_material->m_blend_mode);

			const glm::mat4 mvp = in_projection_matrix * in_view_matrix * it->m_orientation;
			render_mesh(*it, mvp);
		}

		auto current_instanced_begin = instanced_begin;

		while (current_instanced_begin != inout_drawcalls.end())
		{
			auto next_instanced_begin = T_stable_partition ?
			std::stable_partition
			(
				current_instanced_begin, inout_drawcalls.end(),
				[current_instanced_begin](const T_drawcall_type& in_a)
				{
					return current_instanced_begin->m_material == in_a.m_material && current_instanced_begin->m_mesh == in_a.m_mesh;
				}
			)
			:
			std::partition
			(
				current_instanced_begin, inout_drawcalls.end(),
				[current_instanced_begin](const T_drawcall_type& in_a)
				{
					return current_instanced_begin->m_material == in_a.m_material && current_instanced_begin->m_mesh == in_a.m_mesh;
				}
			);

			auto model_matrix_loc_it = current_instanced_begin->m_material->m_instance_data_name_to_offset_lut.find("model_matrix");

			if (model_matrix_loc_it != current_instanced_begin->m_material->m_instance_data_name_to_offset_lut.end())
			{
				GLuint model_matrix_loc = model_matrix_loc_it->second;
				for (auto it = current_instanced_begin; it != next_instanced_begin; ++it)
				{
					memcpy(it->m_instance_data + model_matrix_loc, &it->m_orientation, uniform_block_alignment_functions::get_alignment<glm::mat4x4>());
				}
			}

			auto mvp_loc_it = current_instanced_begin->m_material->m_instance_data_name_to_offset_lut.find("MVP");

			if (mvp_loc_it != current_instanced_begin->m_material->m_instance_data_name_to_offset_lut.end())
			{
				GLuint mvp_loc = mvp_loc_it->second;
				for (auto it = current_instanced_begin; it != next_instanced_begin; ++it)
				{
					const glm::mat4 mvp = in_projection_matrix * in_view_matrix * it->m_orientation;
					memcpy(it->m_instance_data + mvp_loc, &mvp, uniform_block_alignment_functions::get_alignment<glm::mat4x4>());
				}
			}

			if constexpr (T_custom_blend_mode)
				set_blend_mode(current_instanced_begin->m_material->m_blend_mode);

			if (OpenGl_uniform_block_instancing* instancing_block = inout_renderer_resources_state.get_static_uniform_block<OpenGl_uniform_block_instancing>())
			{
				GLfloat instance_data_texture_vec4_stride = (GLfloat)current_instanced_begin->m_material->m_instance_vec4_stride;
				GLuint64 instance_data_tex_handle = current_instanced_begin->m_material->m_instance_data_texture.value().get_bindless_handle();

				instancing_block->set_data_raw(0, sizeof(GLfloat), &instance_data_texture_vec4_stride);
				instancing_block->set_data_raw(uniform_block_alignment_functions::get_alignment<glm::vec4>(), sizeof(GLuint64), &instance_data_tex_handle);

				inout_renderer_resources_state.m_draw_interface_instanced.begin_frame(*current_instanced_begin->m_mesh, *current_instanced_begin->m_material);

				for (auto it = current_instanced_begin; it != next_instanced_begin; ++it)
					inout_renderer_resources_state.m_draw_interface_instanced.draw_instance(it->m_instance_data);

				inout_renderer_resources_state.m_draw_interface_instanced.end_frame();
			}

			current_instanced_begin = next_instanced_begin;
		}
	}
}
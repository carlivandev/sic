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
#include "sic/update_list.h"

#include "glm/mat4x4.hpp"
#include "glm/vec3.hpp"

#include <unordered_map>
#include <optional>
#include <vector>

namespace sic
{
	struct Render_object_window;
	struct Render_object_view;
	struct Render_object_debug_drawer;
	struct Render_object_mesh;

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
		struct Render_all_3d_objects_data
		{
			const Update_list<Render_object_mesh>* m_meshes = nullptr;
			Update_list<Render_object_debug_drawer>* m_debug_drawer = nullptr;
			State_render_scene* m_scene_state = nullptr;
			State_renderer_resources* m_renderer_resources_state = nullptr;
			State_debug_drawing* m_debug_drawer_state = nullptr;

			glm::mat4x4 m_view_mat;
			glm::mat4x4 m_proj_mat;
			glm::vec3 m_view_location;
		};

		virtual void on_created(Engine_context in_context) override;
		virtual void on_engine_finalized(Engine_context in_context) const override;
		virtual void on_engine_tick(Engine_context in_context, float in_time_delta) const override;

	private:
		void render_view(Engine_context in_context, const Render_object_window& in_window, Render_object_view& inout_view) const;
		void render_ui(Engine_context in_context, const Render_object_window& in_window) const;

		void render_all_3d_objects(Render_all_3d_objects_data in_data) const;

		template <typename T_drawcall_type>
		auto sort_instanced(std::vector<T_drawcall_type>& inout_drawcalls) const;

		template <typename T_iterator_type>
		void render_meshes(T_iterator_type in_begin, T_iterator_type in_end, const glm::mat4& in_projection_matrix, const glm::mat4& in_view_matrix) const;

		template <typename T_iterator_type, typename T_pre_render_func_type>
		void render_instanced(T_iterator_type in_begin, T_iterator_type in_end, T_pre_render_func_type pre_render_func, State_renderer_resources& inout_renderer_resources_state) const;

		template<typename T_drawcall_type>
		void render_mesh(const T_drawcall_type& in_dc, const glm::mat4& in_mvp) const;

		void render_views_to_window_backbuffers(const std::unordered_map<sic::Render_object_window*, std::vector<sic::Render_object_view*>>& in_window_to_view_lut) const;

		void apply_parameters(const Asset_material& in_material, const byte* in_instance_data, const OpenGl_program& in_program) const;

		void do_asset_post_loads(Engine_context in_context) const;

	public:
		static void post_load_texture(Asset_texture& out_texture);
		static void post_load_render_target(Asset_render_target& out_rt);
		static void post_load_material(const State_renderer_resources& in_resource_state, Asset_material& out_material);
		static void post_load_mesh(Asset_model::Mesh& inout_mesh);

	private:
		void set_depth_mode(Depth_mode in_to_set) const;
		void set_blend_mode(Material_blend_mode in_to_set) const;
	};

	template<typename T_drawcall_type>
	inline auto System_renderer::sort_instanced(std::vector<T_drawcall_type>& inout_drawcalls) const
	{
		return T_drawcall_type::get_uses_stable_partition() ?
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
	}
	template<typename T_iterator_type>
	inline void System_renderer::render_meshes(T_iterator_type in_begin, T_iterator_type in_end, const glm::mat4& in_projection_matrix, const glm::mat4& in_view_matrix) const
	{
		for (auto it = in_begin; it != in_end; ++it)
		{
			if constexpr (std::iterator_traits<T_iterator_type>::value_type::get_uses_custom_blendmode())
				set_blend_mode(it->m_material->m_blend_mode);

			const glm::mat4 mvp = in_projection_matrix * in_view_matrix * it->m_orientation;
			render_mesh(*it, mvp);
		}
	}
	template<typename T_iterator_type, typename T_pre_render_func_type>
	inline void System_renderer::render_instanced(T_iterator_type in_begin, T_iterator_type in_end, T_pre_render_func_type pre_render_func, State_renderer_resources& inout_renderer_resources_state) const
	{
		auto current_instanced_begin = in_begin;

		while (current_instanced_begin != in_end)
		{
			auto next_instanced_begin = std::iterator_traits<T_iterator_type>::value_type::get_uses_stable_partition() ?
			std::stable_partition
			(
				current_instanced_begin, in_end,
				[current_instanced_begin](const auto& in_a)
				{
					return in_a.partition(*current_instanced_begin);
				}
			)
			:
			std::partition
			(
				current_instanced_begin, in_end,
				[current_instanced_begin](const auto& in_a)
				{
					return in_a.partition(*current_instanced_begin);
				}
			);

			pre_render_func(current_instanced_begin, next_instanced_begin);

			if constexpr (std::iterator_traits<T_iterator_type>::value_type::get_uses_custom_blendmode())
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

	template<typename T_drawcall_type>
	inline void System_renderer::render_mesh(const T_drawcall_type& in_dc, const glm::mat4& in_mvp) const
	{
		in_dc.m_mesh->m_vertex_buffer_array.value().bind();
		in_dc.m_mesh->m_index_buffer.value().bind();

		auto& program = in_dc.m_material->m_program.value();
		program.use();

		if (program.get_uniform_location("MVP"))
			program.set_uniform("MVP", in_mvp);

		if (program.get_uniform_location("model_matrix"))
			program.set_uniform("model_matrix", in_dc.m_orientation);

		apply_parameters(*in_dc.m_material, in_dc.m_instance_data, program);

		OpenGl_draw_strategy_triangle_element::draw(static_cast<GLsizei>(in_dc.m_mesh->m_index_buffer.value().get_max_elements()), 0);
	}
}
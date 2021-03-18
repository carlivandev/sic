#pragma once
#include "sic/opengl_draw_interface_debug_lines.h"
#include "sic/opengl_engine_uniform_blocks.h"
#include "sic/asset_types.h"
#include "sic/state_render_scene.h"
#include "sic/opengl_texture.h"
#include "sic/opengl_draw_interface_instanced.h"

#include "sic/core/system.h"
#include "sic/core/engine.h"
#include "sic/core/double_buffer.h"
#include "sic/core/update_list.h"

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
	struct State_window;

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

	using Processor_renderer =
	Engine_processor
	<
		Processor_flag_read<State_window>,

		Processor_flag_write<State_render_scene>,
		Processor_flag_write<State_assetsystem>,
		Processor_flag_write<State_debug_drawing>,
		Processor_flag_write<State_renderer_resources>
	>;

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

		static void render(Processor_renderer in_processor);

	private:
		static void render_view(Processor_renderer in_processor, const Render_object_view& inout_view, const OpenGl_render_target& in_render_target);
		static void render_ui(Processor_renderer in_processor, const Render_object_window& in_window, const Update_list_id<Render_object_window>& in_window_id);

		static void render_all_3d_objects(Render_all_3d_objects_data in_data);

		template <typename T_drawcall_type>
		static auto sort_instanced(std::vector<T_drawcall_type>& inout_drawcalls);

		template <typename T_iterator_type>
		static void render_meshes(T_iterator_type in_begin, T_iterator_type in_end, const glm::mat4& in_projection_matrix, const glm::mat4& in_view_matrix);

		template <typename T_iterator_type>
		static void render_meshes_instanced(T_iterator_type in_begin, T_iterator_type in_end, const glm::mat4& in_projection_matrix, const glm::mat4& in_view_matrix, State_renderer_resources& inout_renderer_resources_state);

		template <typename T_iterator_type>
		struct Instancing_chunk
		{
			T_iterator_type m_begin;
			T_iterator_type m_end;
		};

		template <typename T_iterator_type>
		static std::vector<Instancing_chunk<T_iterator_type>> gather_instancing_chunks(T_iterator_type in_begin, T_iterator_type in_end);

		template <typename T_iterator_type>
		static void render_instancing_chunk(Instancing_chunk<T_iterator_type> in_chunk, const OpenGl_vertex_buffer_array_base& in_vba, const OpenGl_buffer& in_index_buffer, OpenGl_uniform_block_instancing& inout_instancing_block, OpenGl_draw_interface_instanced& inout_draw_interface);

		template<typename T_drawcall_type>
		static void render_mesh(const T_drawcall_type& in_dc, const glm::mat4& in_mvp);

		static void render_windows_to_backbuffers(const std::vector<Render_object_window>& in_windows);

		static void apply_parameters(const Asset_material& in_material, const byte* in_instance_data, const OpenGl_program& in_program);

		static void do_asset_post_loads(Processor_renderer in_processor);

	public:
		static void post_load_texture(Asset_texture& out_texture);
		static void post_load_render_target(Asset_render_target& out_rt);
		static void post_load_material(const State_renderer_resources& in_resource_state, Asset_material& out_material);
		static void post_load_mesh(Asset_model::Mesh& inout_mesh);
		static void post_load_font(Asset_font& inout_font);

	private:
		static void set_depth_mode(Depth_mode in_to_set);
		static void set_blend_mode(Material_blend_mode in_to_set);
	};

	template<typename T_drawcall_type>
	inline auto System_renderer::sort_instanced(std::vector<T_drawcall_type>& inout_drawcalls)
	{
		if constexpr (T_drawcall_type::get_partition_type() == Drawcall_partition_type::standard)
		{
			return std::partition
			(
				inout_drawcalls.begin(), inout_drawcalls.end(),
				[](const T_drawcall_type& in_a)
				{
					return !in_a.m_material->m_is_instanced;
				}
			);
		}
		else if constexpr (T_drawcall_type::get_partition_type() == Drawcall_partition_type::stable)
		{
			return std::stable_partition
			(
				inout_drawcalls.begin(), inout_drawcalls.end(),
				[](const T_drawcall_type& in_a)
				{
					return !in_a.m_material->m_is_instanced;
				}
			);
		}
		else
		{
			static_assert(false, "Can not sort T_drawcall_type!");
		}
	}
	template<typename T_iterator_type>
	inline void System_renderer::render_meshes(T_iterator_type in_begin, T_iterator_type in_end, const glm::mat4& in_projection_matrix, const glm::mat4& in_view_matrix)
	{
		for (auto it = in_begin; it != in_end; ++it)
		{
			if constexpr (std::iterator_traits<T_iterator_type>::value_type::get_uses_custom_blendmode())
				set_blend_mode(it->m_material->m_blend_mode);

			const glm::mat4 mvp = in_projection_matrix * in_view_matrix * it->m_orientation;
			render_mesh(*it, mvp);
		}
	}

	template<typename T_iterator_type>
	inline void System_renderer::render_meshes_instanced(T_iterator_type in_begin, T_iterator_type in_end, const glm::mat4& in_projection_matrix, const glm::mat4& in_view_matrix, State_renderer_resources& inout_renderer_resources_state)
	{
		OpenGl_uniform_block_instancing* instancing_block = inout_renderer_resources_state.get_static_uniform_block<OpenGl_uniform_block_instancing>();
		if (!instancing_block)
			return;

		auto&& instancing_chunks = gather_instancing_chunks(in_begin, in_end);

		for (auto&& chunk : instancing_chunks)
		{
			auto model_matrix_loc_it = chunk.m_begin->m_material->m_instance_data_name_to_offset_lut.find("model_matrix");

			if (model_matrix_loc_it != chunk.m_begin->m_material->m_instance_data_name_to_offset_lut.end())
			{
				GLuint model_matrix_loc = model_matrix_loc_it->second;
				for (auto it = chunk.m_begin; it != chunk.m_end; ++it)
				{
					memcpy(it->m_instance_data + model_matrix_loc, &it->m_orientation, uniform_block_alignment_functions::get_alignment<glm::mat4x4>());
				}
			}

			auto mvp_loc_it = chunk.m_begin->m_material->m_instance_data_name_to_offset_lut.find("MVP");

			if (mvp_loc_it != chunk.m_begin->m_material->m_instance_data_name_to_offset_lut.end())
			{
				GLuint mvp_loc = mvp_loc_it->second;
				for (auto it = chunk.m_begin; it != chunk.m_end; ++it)
				{
					const glm::mat4 mvp = in_projection_matrix * in_view_matrix * it->m_orientation;
					memcpy(it->m_instance_data + mvp_loc, &mvp, uniform_block_alignment_functions::get_alignment<glm::mat4x4>());
				}
			}

			if (chunk.m_begin->m_material->m_two_sided)
				glDisable(GL_CULL_FACE);
			else
				glEnable(GL_CULL_FACE);

			render_instancing_chunk(chunk, chunk.m_begin->m_mesh->m_vertex_buffer_array.value(), chunk.m_begin->m_mesh->m_index_buffer.value(), *instancing_block, inout_renderer_resources_state.m_draw_interface_instanced);
		}
	}

	template<typename T_iterator_type>
	inline std::vector<System_renderer::Instancing_chunk<T_iterator_type>> System_renderer::gather_instancing_chunks(T_iterator_type in_begin, T_iterator_type in_end)
	{
		std::vector<Instancing_chunk<T_iterator_type>> chunks;

		auto current_instanced_begin = in_begin;

		while (current_instanced_begin != in_end)
		{
			if constexpr (std::iterator_traits<T_iterator_type>::value_type::get_partition_type() == Drawcall_partition_type::standard)
			{
				auto next_instanced_begin = std::partition
				(
					current_instanced_begin, in_end,
					[current_instanced_begin](const auto& in_a)
					{
						return in_a.partition(*current_instanced_begin);
					}
				);

				chunks.push_back({ current_instanced_begin, next_instanced_begin });
				current_instanced_begin = next_instanced_begin;
			}
			else if constexpr (std::iterator_traits<T_iterator_type>::value_type::get_partition_type() == Drawcall_partition_type::stable)
			{
				auto next_instanced_begin = std::stable_partition
				(
					current_instanced_begin, in_end,
					[current_instanced_begin](const auto& in_a)
					{
						return in_a.partition(*current_instanced_begin);
					}
				);

				chunks.push_back({ current_instanced_begin, next_instanced_begin });
				current_instanced_begin = next_instanced_begin;
			}
			else if constexpr (std::iterator_traits<T_iterator_type>::value_type::get_partition_type() == Drawcall_partition_type::disabled)
			{
				auto next_instanced_begin = current_instanced_begin + 1;

				while (next_instanced_begin != in_end && next_instanced_begin->m_material == current_instanced_begin->m_material)
					++next_instanced_begin;

				chunks.push_back({ current_instanced_begin, next_instanced_begin });
				current_instanced_begin = next_instanced_begin;
			}
		}

		return chunks;
	}

	template<typename T_iterator_type>
	inline void System_renderer::render_instancing_chunk(Instancing_chunk<T_iterator_type> in_chunk, const OpenGl_vertex_buffer_array_base& in_vba, const OpenGl_buffer& in_index_buffer, OpenGl_uniform_block_instancing& inout_instancing_block, OpenGl_draw_interface_instanced& inout_draw_interface)
	{
		if constexpr (std::iterator_traits<T_iterator_type>::value_type::get_uses_custom_blendmode())
			set_blend_mode(in_chunk.m_begin->m_material->m_blend_mode);

		GLfloat instance_data_texture_vec4_stride = (GLfloat)in_chunk.m_begin->m_material->m_instance_vec4_stride;
		GLuint64 instance_data_tex_handle = in_chunk.m_begin->m_material->m_instance_data_texture.value().get_bindless_handle();

		inout_instancing_block.set_data_raw(0, sizeof(GLfloat), &instance_data_texture_vec4_stride);
		inout_instancing_block.set_data_raw(uniform_block_alignment_functions::get_alignment<glm::vec4>(), sizeof(GLuint64), &instance_data_tex_handle);

		inout_draw_interface.begin_frame(in_vba, in_index_buffer, *in_chunk.m_begin->m_material);

		for (auto it = in_chunk.m_begin; it != in_chunk.m_end; ++it)
			inout_draw_interface.draw_instance(it->m_instance_data);

		inout_draw_interface.end_frame();
	}

	template<typename T_drawcall_type>
	inline void System_renderer::render_mesh(const T_drawcall_type& in_dc, const glm::mat4& in_mvp)
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
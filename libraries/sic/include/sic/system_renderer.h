#pragma once
#include "sic/system.h"

#include "sic/engine.h"
#include "sic/opengl_draw_interface_debug_lines.h"
#include "sic/opengl_engine_uniform_blocks.h"
#include "sic/asset_types.h"
#include "sic/double_buffer.h"
#include "sic/state_render_scene.h"
#include "sic/opengl_texture.h"

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
}
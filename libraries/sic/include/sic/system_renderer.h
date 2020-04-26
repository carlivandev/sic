#pragma once
#include "sic/system.h"

#include "sic/engine.h"
#include "sic/opengl_draw_interface_debug_lines.h"
#include "sic/opengl_engine_uniform_blocks.h"
#include "sic/asset_types.h"

#include "glm/mat4x4.hpp"

#include <unordered_map>
#include <optional>

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
		std::optional<OpenGl_uniform_block_view> m_uniform_block_view;
		std::optional<OpenGl_uniform_block_lights> m_uniform_block_lights;

		std::optional<OpenGl_program> m_pass_through_program;
		std::optional<OpenGl_vertex_buffer_array
			<
			OpenGl_vertex_attribute_position2D,
			OpenGl_vertex_attribute_texcoord
			>> m_quad_vertex_buffer_array;

		std::optional<OpenGl_buffer> m_quad_indexbuffer;
	};

	struct Drawcall_mesh
	{
		glm::mat4x4 m_orientation;
		Asset_model::Mesh* m_mesh;
		Asset_material* m_material;
	};

	struct Drawcall_mesh_translucent : Drawcall_mesh
	{
		float m_distance_to_view_2;
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

		static void post_load_texture(Asset_texture& out_texture);
		static void post_load_material(const State_renderer_resources& in_resource_state, Asset_material& out_material);
		static void post_load_mesh(Asset_model::Mesh& inout_mesh);

		void set_depth_mode(Depth_mode in_to_set) const;
		void set_blend_mode(Material_blend_mode in_to_set) const;
	};
}
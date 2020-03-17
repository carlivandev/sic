#pragma once
#include "sic/engine_context.h"
#include "sic/update_list.h"

#include "sic/gl_includes.h"
#include "sic/asset.h"
#include "sic/render_target.h"

#include "glm/mat4x4.hpp"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"

#include <optional>

namespace sic
{
	struct Asset_model;
	struct Asset_material;
	struct Object_window;

	struct Render_object_view : Noncopyable
	{
		GLFWwindow* m_window_render_on = nullptr;

		glm::mat4x4 m_view_orientation = glm::mat4x4(1);

		//offset in percentage(0 - 1) based on window size, top-left
		glm::vec2 m_viewport_offset = { 0.0f, 0.0f };
		//size in percentage(0 - 1) based on window size, top-left
		glm::vec2 m_viewport_size = { 1.0f, 1.0f };

		float m_fov = 45.0f;
		float m_near_plane = 0.1f;
		float m_far_plane = 100.0f;

		Render_target m_render_target;
		i32 m_level_id = -1;
	};

	struct Render_object_model
	{
		Asset_ref<Asset_model> m_model;
		std::unordered_map<std::string, Asset_ref<Asset_material>> m_material_overrides;
		glm::vec3 m_position = { 0.0f, 0.0f, 0.0f };
	};

	enum struct Debug_shape
	{
		box,
		sphere,
		none
	};

	struct Render_object_debug_shape
	{
		Debug_shape m_shape = Debug_shape::box;
		//use life time to push to destroy post render if < 0
		float m_lifetime = -1.0f;
	};

	template <typename t_type>
	struct Render_object_id
	{
		Update_list_id<t_type> m_id;
		i32 m_level_id = -1;
	};

	struct State_render_scene : State
	{
		friend struct System_renderer;
		friend struct System_renderer_state_swapper;

		using Render_scene_level =
			std::tuple
			<
			Update_list<Render_object_model>,
			Update_list<Render_object_debug_shape>
			>;

		template <typename t_type>
		Render_object_id<t_type> create_object(i32 in_level_id, Update_list<t_type>::Update::template Callback&& in_create_callback)
		{
			Update_list<t_type>& list = std::get<Update_list<t_type>>(m_level_id_to_scene_lut[in_level_id]);
			Render_object_id<t_type> id;
			id.m_id = list.create_object(std::move(in_create_callback));
			id.m_level_id = in_level_id;

			return id;
		}

		template <typename t_type>
		void update_object(Render_object_id<t_type> in_object_id, Update_list<t_type>::Update::template Callback&& in_update_callback)
		{
			Update_list<t_type>& list = std::get<Update_list<t_type>>(m_level_id_to_scene_lut[in_object_id.m_level_id]);
			list.update_object(in_object_id.m_id, std::move(in_update_callback));
		}

		template <typename t_type>
		void destroy_object(Render_object_id<t_type> in_object_id)
		{
			Update_list<t_type>& list = std::get<Update_list<t_type>>(m_level_id_to_scene_lut[in_object_id.m_level_id]);
			list.destroy_object(in_object_id.m_id);
		}

		Update_list<Render_object_view> m_views;

	protected:
		void flush_updates();

		std::unordered_map<i32, Render_scene_level> m_level_id_to_scene_lut;
	};
}
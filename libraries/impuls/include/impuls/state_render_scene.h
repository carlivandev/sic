#pragma once
#include "impuls/engine_context.h"
#include "impuls/update_list.h"

#include "impuls/asset.h"
#include "impuls/gl_includes.h"

#include "glm/mat4x4.hpp"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"

namespace sic
{
	struct Asset_model;
	struct Asset_material;
	struct Object_window;

	struct Render_object_view
	{
		GLFWwindow* m_window_render_on = nullptr;

		glm::mat4x4 m_view_orientation;

		//offset in percentage(0 - 1) based on window size, top-left
		glm::vec2 m_viewport_offset = { 0.0f, 0.0f };
		//size in percentage(0 - 1) based on window size, top-left
		glm::vec2 m_viewport_size = { 1.0f, 1.0f };

		GLuint m_framebuffer_id = 0;
		GLuint m_texture_id = 0;
		GLuint m_depth_texture_id = 0;
	};

	struct Render_object_model
	{
		Asset_ref<Asset_model> m_model;
		std::unordered_map<std::string, Asset_ref<Asset_material>> m_material_overrides;
		glm::vec3 m_position = { 0.0f, 0.0f, 0.0f };
	};

	enum class Debug_shape
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

	struct State_render_scene : State
	{
		friend struct System_renderer;
		friend struct System_renderer_state_swapper;

		Update_list<Render_object_view> m_views;
		Update_list<Render_object_model> m_models;
		Update_list<Render_object_debug_shape> m_debug_shapes;

	protected:
		void flush_updates();
	};
}
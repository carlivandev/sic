#include "sic/system_renderer_state_swapper.h"
#include "sic/system_renderer.h"

#include "sic/state_render_scene.h"
#include "sic/system_window.h"

void sic::System_renderer_state_swapper::on_engine_tick(Engine_context in_context, float in_time_delta) const
{
	in_time_delta;

	State_render_scene* scene_state = in_context.get_state<State_render_scene>();

	if (!scene_state)
		return;

	State_window* window_state = in_context.get_state<State_window>();

	if (!window_state)
		return;

	if (!window_state->m_main_window)
		return;

	glfwMakeContextCurrent(window_state->m_resource_context);

	scene_state->flush_updates();
}
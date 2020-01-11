#include "sic/system_renderer_state_swapper.h"
#include "sic/system_renderer.h"

#include "sic/state_render_scene.h"

void sic::System_renderer_state_swapper::on_engine_tick(Engine_context&& in_context, float in_time_delta) const
{
	in_time_delta;

	State_render_scene* scene_state = in_context.get_state<State_render_scene>();

	if (!scene_state)
		return;

	scene_state->flush_updates();
}
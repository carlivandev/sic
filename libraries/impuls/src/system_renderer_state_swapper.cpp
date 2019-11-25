#include "impuls/system_renderer_state_swapper.h"
#include "impuls/system_renderer.h"

#include "impuls/state_render_scene.h"

void impuls::system_renderer_state_swapper::on_tick(world_context&& in_context, float in_time_delta) const
{
	in_time_delta;

	state_render_scene* scene_state = in_context.get_state<state_render_scene>();

	if (!scene_state)
		return;

	scene_state->flush_updates();
}
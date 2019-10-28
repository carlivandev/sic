#include "impuls/system_renderer_state_swapper.h"
#include "impuls/system_renderer.h"

void impuls::system_renderer_state_swapper::on_tick(world_context&& in_context, float in_time_delta) const
{
	in_time_delta;

	state_renderer* renderer_state = in_context.get_state<state_renderer>();

	if (!renderer_state)
		return;

	renderer_state->m_model_drawcalls.swap
	(
		[](std::vector<drawcall_model>& in_read_models, std::vector<drawcall_model>& in_write_models)
		{
			in_read_models.clear();
		}
	);
}
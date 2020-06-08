#include "sic/system_renderer_state_swapper.h"
#include "sic/system_renderer.h"
#include "sic/system_asset.h"

#include "sic/state_render_scene.h"
#include "sic/system_window.h"

void sic::System_renderer_state_swapper::on_engine_tick(Engine_context in_context, float in_time_delta) const
{
	in_time_delta;

	State_render_scene& scene_state = in_context.get_state_checked<State_render_scene>();
	State_window& window_state = in_context.get_state_checked<State_window>();
	State_renderer& renderer_state = in_context.get_state_checked<State_renderer>();
	State_assetsystem& assetsystem_state = in_context.get_state_checked<State_assetsystem>();

	std::scoped_lock asset_modification_lock(assetsystem_state.get_modification_mutex());

	glfwMakeContextCurrent(window_state.m_resource_context);

	scene_state.flush_updates();

	renderer_state.m_synchronous_renderer_updates.read
	(
		[&context = in_context](const std::vector<std::function<void(Engine_context&)>>& in_updates)
		{
			for (auto&& update : in_updates)
				update(context);
		}
	);

	renderer_state.m_synchronous_renderer_updates.swap
	(
		[](std::vector<std::function<void(Engine_context&)>>& in_out_read, std::vector<std::function<void(Engine_context&)>>&)
		{
			in_out_read.clear();
		}
	);
}
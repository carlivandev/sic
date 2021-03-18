#include "sic/system_renderer_state_swapper.h"
#include "sic/system_renderer.h"
#include "sic/system_asset.h"

#include "sic/state_render_scene.h"
#include "sic/system_window.h"

void sic::System_renderer_state_swapper::on_engine_tick(Engine_context in_context, float in_time_delta) const
{
	in_time_delta;

	in_context.schedule
	(std::function([](Engine_processor<Processor_flag_write<State_render_scene>, Processor_flag_read<State_window>, Processor_flag_write<State_assetsystem>> in_processor)
	{
		std::scoped_lock asset_modification_lock(in_processor.get_state_checked_w<State_assetsystem>().get_modification_mutex());

		glfwMakeContextCurrent(in_processor.get_state_checked_r<State_window>().m_resource_context);
		in_processor.get_state_checked_w<State_render_scene>().flush_updates();
	}), Schedule_data().run_on_main_thread(true));

	State_renderer& renderer_state = in_context.get_state_checked<State_renderer>();

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
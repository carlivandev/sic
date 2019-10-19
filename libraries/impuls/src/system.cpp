#include "impuls/system.h"
#include "impuls/world_context.h"

void impuls::i_system::execute_tick(world_context&& in_context, float in_time_delta) const
{
	//printf("system tick: \"%s\"\n", m_name.c_str());
	on_tick(std::move(in_context), in_time_delta);

	for (i_system* subsystem : m_subsystems)
		subsystem->execute_tick(world_context(*in_context.m_world, *subsystem), in_time_delta);
}
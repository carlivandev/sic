#include "impuls/system.h"
#include "impuls/engine_context.h"
#include "impuls/level_context.h"

void impuls::i_system::execute_tick(level_context&& in_context, float in_time_delta) const
{
	//printf("system tick: \"%s\"\n", m_name.c_str());
	on_tick(level_context(in_context.m_engine, in_context.m_level), in_time_delta);

	for (i_system* subsystem : m_subsystems)
		subsystem->execute_tick(level_context(in_context.m_engine, in_context.m_level), in_time_delta);
}

void impuls::i_system::execute_engine_tick(engine_context&& in_context, float in_time_delta) const
{
	//printf("system engine tick: \"%s\"\n", m_name.c_str());
	on_engine_tick(engine_context(in_context.m_engine), in_time_delta);

	for (i_system* subsystem : m_subsystems)
		subsystem->on_engine_tick(engine_context(in_context.m_engine), in_time_delta);
}

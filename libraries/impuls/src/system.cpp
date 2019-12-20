#include "impuls/system.h"
#include "impuls/engine_context.h"
#include "impuls/level_context.h"

void sic::System::execute_tick(Level_context&& in_context, float in_time_delta) const
{
	//printf("system tick: \"%s\"\n", m_name.c_str());
	on_tick(Level_context(in_context.m_engine, in_context.m_level), in_time_delta);

	for (System* subsystem : m_subsystems)
		subsystem->execute_tick(Level_context(in_context.m_engine, in_context.m_level), in_time_delta);
}

void sic::System::execute_engine_tick(Engine_context&& in_context, float in_time_delta) const
{
	//printf("system engine tick: \"%s\"\n", m_name.c_str());
	on_engine_tick(Engine_context(in_context.m_engine), in_time_delta);

	for (System* subsystem : m_subsystems)
		subsystem->on_engine_tick(Engine_context(in_context.m_engine), in_time_delta);
}

#include "sic/system.h"
#include "sic/engine_context.h"
#include "sic/level_context.h"

void sic::System::on_created(Engine_context)
{
}

void sic::System::on_shutdown(Engine_context)
{
}

void sic::System::on_begin_simulation(Level_context) const
{
}

void sic::System::on_tick(Level_context, float) const
{
}

void sic::System::on_end_simulation(Level_context) const
{
}

void sic::System::on_engine_tick(Engine_context, float) const
{
}

void sic::System::execute_tick(Level_context in_context, float in_time_delta) const
{
	//printf("system tick: \"%s\"\n", m_name.c_str());
	on_tick(in_context, in_time_delta);

	for (System* subsystem : m_subsystems)
		subsystem->execute_tick(in_context, in_time_delta);
}

void sic::System::execute_engine_tick(Engine_context in_context, float in_time_delta) const
{
	//printf("system engine tick: \"%s\"\n", m_name.c_str());
	on_engine_tick(in_context, in_time_delta);

	for (System* subsystem : m_subsystems)
		subsystem->on_engine_tick(in_context, in_time_delta);
}

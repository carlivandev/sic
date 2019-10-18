#pragma once
#include "impuls/system.h"
#include "impuls/world.h"

#include <vector>

#include <Windows.h>

struct state_shape_renderer : public impuls::i_state
{
	HANDLE m_console_handle = nullptr;

	std::vector<wchar_t> m_screen_buffer;
};

struct shape_renderer_system : public impuls::i_system
{
	virtual void on_created(impuls::world_context&& in_context) const override;
	virtual void on_begin_simulation(impuls::world_context&& in_context) const override;
	virtual void on_tick(impuls::world_context&& in_context, float in_time_delta) const override;
	virtual void on_end_simulation(impuls::world_context&& in_context) const override;

	void draw_shapes(const impuls::world_context& in_context, state_shape_renderer& in_state) const;
};


#pragma once
#include "impuls/system.h"
#include "impuls/world.h"

#include "shape.h"

struct state_shape_controller : public impuls::i_state
{
	shape_data* m_controlled_shape = nullptr;
	float m_time_until_fall = 1.0f;
	float m_cur_time_until_fall = m_time_until_fall;
};

struct shape_controller_system : public impuls::i_system
{
	virtual void on_created(impuls::world_context&& in_context) const override;
	virtual void on_tick(impuls::world_context&& in_context, float in_time_delta) const override;

	bool try_move(shape_data& in_shape, impuls::i32 in_x_move, impuls::i32 in_y_move) const;
};


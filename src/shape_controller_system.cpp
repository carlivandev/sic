#include "pch.h"
#include "shape_controller_system.h"
#include "impuls/input.h"

#include "impuls/world_context.h"
#include <random>

void shape_controller_system::on_created(impuls::world_context&& in_context) const
{
	in_context.register_state<state_shape_controller>("state_shape_controller");
}

void shape_controller_system::on_tick(impuls::world_context&& in_context, float in_time_delta) const
{
	state_shape_controller* controller_state = in_context.get_state<state_shape_controller>();

	if (!controller_state)
		return;

	if (controller_state->m_controlled_shape == nullptr)
	{
		controller_state->m_controlled_shape = &in_context.create_object_instance<shape>().get<shape_data>();

		static bool init = false;
		static std::random_device rd;
		static std::mt19937 eng;
		if (!init) {
			eng.seed(rd()); // Seed random engine
			init = true;
		}

		std::uniform_int_distribution<int> shape_dist(0, 6);

		controller_state->m_controlled_shape->m_pos_x = 5;
		controller_state->m_controlled_shape->m_pos_y = 4;
		controller_state->m_controlled_shape->shape_idx = shape_dist(eng);
		controller_state->m_controlled_shape->rotation_idx = 3;
	}

	controller_state->m_cur_time_until_fall -= in_time_delta;

	if (controller_state->m_cur_time_until_fall <= 0.0f)
	{
		controller_state->m_cur_time_until_fall = controller_state->m_time_until_fall;

		if (!try_move(*controller_state->m_controlled_shape, 0, 1))
		{
			controller_state->m_controlled_shape = nullptr;
			return;
		}
	}
}

bool shape_controller_system::try_move(shape_data & in_shape, impuls::i32 in_x_move, impuls::i32 in_y_move) const
{
	in_shape.m_pos_x += in_x_move;
	in_shape.m_pos_y += in_y_move;
	return true;
}

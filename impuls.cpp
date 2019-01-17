#include "pch.h"
#include <iostream>
#include <vector>
#include <unordered_map>
#include <functional>
#include "world.h"
#include "component_storage_handle.h"


struct collider : public i_component
{
	std::function<void(collider& in_col, collider& in_other)> m_on_begin_overlap;
	float m_value = 1.0f;
};

struct player : public i_component
{
	float x, y, z;
};

struct collision_system : public i_world_system
{
	virtual void on_tick(world& in_world, float in_time_delta) override
	{
		collider test;

		for (collider& instance : in_world.all<collider>())
		{
			if (instance.m_on_begin_overlap)
				instance.m_on_begin_overlap(instance, test);
		}
	}
};

struct player_system : public i_world_system
{
	virtual void on_created(world& in_world)
	{
		m_world = &in_world;

		entity& new_entity = in_world.create_entity();
		m_player = &in_world.create_component<player>(new_entity);
		collider& col = in_world.create_component<collider>(new_entity);

		col.m_on_begin_overlap = std::bind(&player_system::on_player_collide, this, std::placeholders::_1, std::placeholders::_2);
	}

	void on_player_collide(collider& in_col, collider& in_other)
	{
		if (m_world->get_component<player>(*in_col.m_owner))
			std::cout << "player is colliding\n";
	}

	player* m_player = nullptr;
	world* m_world = nullptr;
};

int main()
{
	world world;
	world.initialize();

	world.register_component_type<collider>();
	world.register_component_type<player>();

	world.create_system<collision_system>();
	world.create_system<player_system>();

	while (!world.is_destroyed())
		world.simulate();
}
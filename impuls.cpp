#include "pch.h"
#include <iostream>
#include <vector>
#include <unordered_map>
#include <functional>
#include "world.h"
#include "component_storage_handle.h"


struct collider;

struct collision
{
	collision(collider& in_this, collider& in_other) : m_this(in_this), m_other(in_other) {}
	collider& m_this;
	collider& m_other;
};

struct collider : public i_component
{
	std::function<void(collision&&)> m_on_begin_overlap;
	float radius = 1.0f;
};

struct player : public i_component
{
	float attack_damage;
};

struct collision_system : public i_world_system
{
	virtual void on_tick(float in_time_delta) override
	{
		for (collider& instance : m_world->all<collider>())
		{
			for (collider& other_instance : m_world->all<collider>())
			{
				if (instance.m_instance_index == other_instance.m_instance_index)
					continue;

				if (instance.m_on_begin_overlap && overlaps(instance, other_instance))
					instance.m_on_begin_overlap(collision(instance, other_instance));
			}
		}
	}

	bool overlaps(const collider& in_first, const collider& in_second) const
	{
		if (in_first.radius == in_second.radius)
			return true;

		return false;
	}
};

struct player_system : public i_world_system
{
	virtual void on_created()
	{
		entity& new_entity = m_world->create_entity();
		m_player = &m_world->create_component<player>(new_entity);
		collider& col = m_world->create_component<collider>(new_entity);

		col.m_on_begin_overlap = std::bind(&player_system::on_player_collide, this, std::placeholders::_1);
	}

	void on_player_collide(collision&& in_collision)
	{
		if (m_world->get_component<player>(*in_collision.m_this.m_owner))
			std::cout << "player is colliding\n";

		m_world->destroy_entity(*in_collision.m_other.m_owner);
	}

	player* m_player = nullptr;
};

int main()
{
	world world;
	world.initialize();

	world.register_component_type<collider>();
	world.register_component_type<player>();

	world.create_system<collision_system>();
	world.create_system<player_system>();

	entity& ent = world.create_entity();
	world.create_component<collider>(ent).radius = 0.0f;



	while (!world.is_destroyed())
		world.simulate();
}
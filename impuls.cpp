#include "pch.h"
#include "world.h"

#include <iostream>

struct collider;

struct collision
{
	collision(collider& in_this, collider& in_other) : m_this(in_this), m_other(in_other) {}
	collider& m_this;
	collider& m_other;
};

struct on_begin_overlap : public impuls::event<collision> {} ;

struct collider : public impuls::i_component
{
	on_begin_overlap m_on_begin_overlap;
	
	float radius = 1.0f;
};

struct player : public impuls::i_component
{
	float attack_damage;
};

struct collision_system : public impuls::i_world_system
{
	virtual void on_tick(float in_time_delta) override
	{
		in_time_delta;

		for (collider& instance : m_world->all<collider>())
		{
			for (collider& other_instance : m_world->all<collider>())
			{
				if (instance.m_instance_index == other_instance.m_instance_index)
					continue;

				if (overlaps(instance, other_instance))
				{
					collision col(instance, other_instance);
					instance.m_on_begin_overlap.invoke(col);
				}
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

struct player_system : public impuls::i_world_system
{
	virtual void on_created()
	{
		m_world->listen<impuls::on_component_created<player>>
		(
			[this](player& in_player)
			{
				on_player_created(in_player);
			}
		);

		m_world->listen<impuls::on_component_destroyed<player>>
		(
			[this](player& in_player)
			{
				on_player_destroyed(in_player);
			}
		);
		
		impuls::entity& new_entity = m_world->create_entity();
		m_player = &m_world->create_component<player>(new_entity);
		collider& col = m_world->create_component<collider>(new_entity);

		col.m_on_begin_overlap.add_listener([this](collision& in_col) {on_player_collide(in_col); });
	}

	void on_player_collide(collision& in_collision)
	{
		if (m_world->get_component<player>(*in_collision.m_this.m_owner))
			std::cout << "player is colliding\n";

		m_world->destroy_component(in_collision.m_this);
	}

	void on_player_created(player& in_player)
	{
		in_player;
	}

	void on_player_destroyed(player& in_player)
	{
		in_player;
	}

	player* m_player = nullptr;
};

int main()
{
	impuls::world world;
	world.initialize();

	world.register_component_type<collider>();
	world.register_component_type<player>();

	world.create_system<collision_system>();
	world.create_system<player_system>();

	impuls::entity& ent = world.create_entity();
	world.create_component<collider>(ent).radius = 0.0f;
	world.create_component<collider>(ent).radius = 1.0f;
	world.create_component<collider>(ent).radius = 2.0f;
	world.create_component<collider>(ent).radius = 3.0f;


	while (!world.is_destroyed())
		world.simulate();
}
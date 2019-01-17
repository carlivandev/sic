#pragma once
#include "pch.h"

#include <new>

#include "component.h"
#include "component_view.h"
#include <chrono>

struct world;

struct i_world_system
{
	virtual ~i_world_system() = default;

	/*
		happens right after a system has been created in a world
		useful for creating subsystems
	*/
	virtual void on_created(world& in_world) {}

	/*
		happens after world has finished setting up
	*/
	virtual void on_begin_simulation(world& world_) {}

	/*
		happens after world has called on_begin_simulation
		called every frame
	*/
	virtual void on_tick(world& in_world, float in_time_delta) {}

	/*
		happens when world is destroyed
	*/
	virtual void on_end_simulation(world& in_world) {}

	void set_enabled(bool in_val)
	{
		m_enabled = in_val;
	}

	bool is_enabled() const { return m_enabled; }

	bool m_enabled = true;

};

struct world
{
	~world()
	{
		for (auto& system : m_systems)
		{
			delete system;
			system = nullptr;
		}
	}

	void initialize(size_t in_initial_entity_capacity = 128, size_t in_entity_bucket_capacity = 64);

	void simulate();
	void begin_simulation();
	void tick();
	void end_simulation();

	void destroy() { m_is_destroyed = true; }
	bool is_destroyed() const { return m_is_destroyed; }

	template <typename component_type>
	void register_component_type(size_t in_initial_capacity = 128, size_t in_bucket_capacity = 64);

	template <typename system_type>
	system_type& create_system();

	entity& create_entity();

	void destroy_entity(entity& in_entity_to_destroy);

	template <typename component_type>
	component_type& create_component(entity& in_entity_to_attach_to);

	template <typename component_type>
	void destroy_component(component_type& in_component_to_destroy);

	template <typename component_type>
	component_type* get_component(entity& in_entity_to_get_from);

	template <typename component_type>
	component_view<component_type, component_storage> all();

	template <typename component_type>
	const component_view<component_type, const component_storage> all() const;

	void refresh_time_delta();

	std::vector<component_storage*> m_component_storages;
	std::vector<i_world_system*> m_systems;
	bucket_allocator<entity> m_entities;
	std::vector<size_t> m_entity_free_indices;

	std::chrono::time_point<std::chrono::high_resolution_clock> m_previous_frame_time_point;
	float m_time_delta = -1.0f;

	bool m_is_destroyed = true;
	bool m_initialized = false;
	bool m_begun_simulation = false;
};

inline void world::initialize(size_t in_initial_entity_capacity, size_t in_entity_bucket_capacity)
{
	assert(!m_initialized && "");

	m_entities.allocate(in_initial_entity_capacity, in_entity_bucket_capacity);

	m_initialized = true;
	m_is_destroyed = false;
}

inline void world::simulate()
{
	refresh_time_delta();

	if (is_destroyed())
		return;

	if (!m_initialized)
		return;

	if (!m_begun_simulation)
	{
		begin_simulation();
		m_begun_simulation = true;
		refresh_time_delta();
	}

	if (!is_destroyed())
		tick();

	if (is_destroyed())
		end_simulation();
}

inline void world::begin_simulation()
{
	for (auto system : m_systems)
	{
		if (!m_is_destroyed)
			system->on_begin_simulation(*this);
	}

	m_begun_simulation = true;
}

inline void world::tick()
{
	for (auto system : m_systems)
	{
		if (!m_is_destroyed && system->is_enabled())
			system->on_tick(*this, m_time_delta);
	}
}

inline void world::end_simulation()
{
	for (auto system : m_systems)
	{
		if (!m_is_destroyed)
			system->on_end_simulation(*this);
	}
	
	this->~world();
	new (this) world();

}

template<typename component_type>
inline void world::register_component_type(size_t in_initial_capacity, size_t in_bucket_capacity)
{
	const size_t type_index = component_storage::component_type_index<component_type>();

	while (type_index >= m_component_storages.size())
		m_component_storages.push_back(new component_storage());

	m_component_storages[type_index]->initialize<component_type>(in_initial_capacity, in_bucket_capacity);
}

template<typename system_type>
inline system_type& world::create_system()
{
	static_assert(std::is_base_of<i_world_system, system_type>::value, "system_type must derive from struct i_world_system");

	system_type* new_system = new system_type();

	m_systems.push_back(new_system);
	new_system->on_created(*this);

	return *new_system;
}

inline entity& world::create_entity()
{
	if (!m_entity_free_indices.empty())
	{
		entity& new_entity = m_entities[m_entity_free_indices.back()];
		new_entity.m_instance_index = m_entity_free_indices.back();

		m_entity_free_indices.pop_back();

		return new_entity;
	}

	entity& new_entity = m_entities.emplace_back();
	new (&new_entity) entity();

	new_entity.m_instance_index = m_entities.size() - 1;

	return new_entity;
}

inline void world::destroy_entity(entity& in_entity_to_destroy)
{
	for (auto& component_handle : in_entity_to_destroy.m_components)
	{
		const int instance_index = component_handle.m_component->m_instance_index;
		component_storage& storage = *m_component_storages[component_handle.m_type_index];
		byte& byte = storage.m_components[instance_index * storage.m_component_type_size];

		storage.m_free_indices.push_back(instance_index);

		i_component* component = reinterpret_cast<i_component*>(&byte);
		component->m_instance_index = -1;
		component->m_owner = nullptr;
	}

	m_entity_free_indices.push_back(in_entity_to_destroy.m_instance_index);

	in_entity_to_destroy.m_components.clear();
	in_entity_to_destroy.m_instance_index = -1;
}

inline void world::refresh_time_delta()
{
	const auto now = std::chrono::high_resolution_clock::now();
	const auto dif = now - m_previous_frame_time_point;
	m_time_delta = std::chrono::duration<float, std::milli>(dif).count() * 0.001f;
	m_previous_frame_time_point = now;
}

template<typename component_type>
inline component_type& world::create_component(entity& in_entity_to_attach_to)
{
	const size_t type_index = component_storage::component_type_index<component_type>();

	component_type& new_instance = m_component_storages[type_index]->create_component<component_type>();
	new_instance.m_owner = &in_entity_to_attach_to;

	in_entity_to_attach_to.m_components.push_back(component_handle(type_index, &new_instance));

	return new_instance;
}

template<typename component_type>
inline void world::destroy_component(component_type& in_component_to_destroy)
{
	const size_t type_index = component_storage::component_type_index<component_type>();
	m_component_storages[type_index]->destroy_component<component_type>(in_component_to_destroy);
}

template<typename component_type>
inline component_type* world::get_component(entity& in_entity_to_get_from)
{
	const size_t type_index = component_storage::component_type_index<component_type>();

	for (component_handle& handle : in_entity_to_get_from.m_components)
	{
		if (handle.m_type_index == type_index)
			return reinterpret_cast<component_type*>(&m_component_storages[type_index]->m_components[handle.m_component->m_instance_index * sizeof(component_type)]);
	}

	return nullptr;
}

template<typename component_type>
inline component_view<component_type, component_storage> world::all()
{
	const size_t type_index = component_storage::component_type_index<component_type>();
	return component_view<component_type, component_storage>(m_component_storages[type_index]);
}

template<typename component_type>
inline const component_view<component_type, const component_storage> world::all() const
{
	const size_t type_index = component_storage::component_type_index<component_type>();
	return component_view<component_type, const component_storage>(m_component_storages[type_index]);
}
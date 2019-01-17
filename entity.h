#pragma once
#include "pch.h"

struct i_component;

struct component_handle
{
	component_handle(size_t in_type_index, i_component* in_component) : m_type_index(in_type_index), m_component(in_component) {}
	size_t m_type_index;
	i_component* m_component;
};
struct entity
{
	bool is_valid() const { return m_instance_index != -1; }
	std::vector<component_handle> m_components;
	int m_instance_index = -1;
};
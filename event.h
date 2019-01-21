#pragma once
#include "pch.h"

struct world_event_base
{
	virtual ~world_event_base() = default;
	virtual void invoke(void* in_data) = 0;
};

template <typename data_to_send>
struct world_event : public world_event_base
{
	typedef data_to_send argument_type;

	void invoke(void* in_data) override
	{
		invoke(*reinterpret_cast<data_to_send*>(in_data));
	}

	void invoke(data_to_send& in_data)
	{
		for (auto& func : m_listeners)
		{
			func(in_data);
		}
	}

	template <typename functor>
	size_t add_listener(functor in_func)
	{
		for (size_t i = 0; i < m_listeners.size(); i++)
		{
			if (m_listeners[i] == nullptr)
			{
				m_listeners[i] = in_func;
				return i;
			}
		}

		m_listeners.push_back(in_func);
		return m_listeners.size() - 1;
	}

	void remove_listener(size_t in_listener_index)
	{
		m_listeners[in_listener_index] = nullptr;
	}

	std::vector<std::function<void(data_to_send&)>> m_listeners;
};

struct world_event_index
{
	template <typename event_type>
	static size_t get()
	{
		static size_t index = m_event_index_ticker++;
		return index;
	}
	static size_t m_event_index_ticker;
};

size_t world_event_index::m_event_index_ticker = 0;

template <typename component_type>
struct on_component_created : public world_event<component_type> {};

template <typename component_type>
struct on_component_destroyed : public world_event<component_type> {};
#pragma once
#include "pch.h"

#include <functional>

namespace impuls
{
	struct event_base
	{
		virtual ~event_base() = default;
		virtual void invoke(void* in_data) = 0;
	};

	template <typename data_to_send>
	struct event : public event_base
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
		ui32 add_listener(functor in_func)
		{
			for (ui32 i = 0; i < m_listeners.size(); i++)
			{
				if (m_listeners[i] == nullptr)
				{
					m_listeners[i] = in_func;
					return i;
				}
			}

			m_listeners.push_back(in_func);
			return static_cast<ui32>(m_listeners.size() - 1);
		}

		void remove_listener(ui32 in_listener_index)
		{
			m_listeners[in_listener_index] = nullptr;
		}

		std::vector<std::function<void(data_to_send&)>> m_listeners;
	};

	template <typename component_type>
	struct on_component_created : public event<component_type> {};

	template <typename component_type>
	struct on_component_destroyed : public event<component_type> {};
}
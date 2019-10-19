#pragma once

#include <functional>

namespace impuls
{
	struct i_event_base
	{
		virtual ~i_event_base() = default;
		virtual void invoke(void* in_data) = 0;
	};

	template <typename t_data_to_send>
	struct i_event : public i_event_base
	{
		typedef t_data_to_send argument_type;

		void invoke(void* in_data) override
		{
			invoke(*reinterpret_cast<t_data_to_send*>(in_data));
		}

		void invoke(t_data_to_send& in_data)
		{
			for (i32 i = 0; i < m_listeners.size(); i++)
			{
				m_listeners[i](in_data);
			}
		}

		template <typename t_functor>
		ui32 add_listener(t_functor in_func)
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

		std::vector<std::function<void(t_data_to_send&)>> m_listeners;
	};

	template <typename object_type>
	struct event_created : public i_event<object_type> {};

	template <typename object_type>
	struct event_destroyed : public i_event<object_type> {};

	//called after all components on an object has been created, best place to bind references
	template <typename object_type>
	struct event_post_created : public i_event<object_type> {};
}
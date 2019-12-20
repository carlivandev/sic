#pragma once

#include <functional>

namespace sic
{
	struct Engine_context;

	struct Event_base
	{
		virtual ~Event_base() = default;
		virtual void invoke(Engine_context&& in_out_context, void* in_data) = 0;
	};

	template <typename t_data_to_send>
	struct Event : public Event_base
	{
		typedef t_data_to_send argument_type;

		void invoke(Engine_context&& in_out_context, void* in_data) override
		{
			invoke(std::move(in_out_context), *reinterpret_cast<t_data_to_send*>(in_data));
		}

		void invoke(Engine_context&& in_out_context, t_data_to_send& in_data)
		{
			for (i32 i = 0; i < m_listeners.size(); i++)
			{
				m_listeners[i](in_out_context, in_data);
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

		std::vector<std::function<void(Engine_context&, t_data_to_send&)>> m_listeners;
	};

	template <typename object_type>
	struct event_created : public Event<object_type> {};

	template <typename object_type>
	struct event_destroyed : public Event<object_type> {};

	//called after all components on an object has been created, best place to bind references
	template <typename object_type>
	struct event_post_created : public Event<object_type> {};
}
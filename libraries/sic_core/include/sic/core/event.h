#pragma once
#include "sic/core/type_restrictions.h"

#include <functional>

namespace sic
{
	struct Engine_context;

	struct Event_base : Noncopyable
	{
		virtual ~Event_base() = default;
		virtual void invoke(Engine_context in_out_context, void* in_data) = 0;
	};

	template <typename T_data_to_send>
	struct Event : public Event_base
	{
		void invoke(Engine_context in_out_context, void* in_data) override
		{
			invoke(std::move(in_out_context), *reinterpret_cast<T_data_to_send*>(in_data));
		}

		void invoke(Engine_context in_out_context, T_data_to_send& in_data)
		{
			for (i32 i = 0; i < m_listeners.size(); i++)
			{
				m_listeners[i](in_out_context, in_data);
			}
		}

		template <typename T_functor>
		ui32 add_listener(T_functor in_func)
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

		std::vector<std::function<void(Engine_context&, T_data_to_send&)>> m_listeners;
	};

	template <typename object_type>
	struct Event_created : public Event<std::reference_wrapper<object_type>> {};

	template <typename object_type>
	struct Event_destroyed : public Event<std::reference_wrapper<object_type>> {};

	//called after all components on an object has been created, best place to bind references
	template <typename object_type>
	struct Event_post_created : public Event<std::reference_wrapper<object_type>> {};
}
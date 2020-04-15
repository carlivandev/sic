#pragma once
#include "sic/type_restrictions.h"

#include <vector>
#include <functional>
#include <mutex>

namespace sic
{
	template <typename ...t_args>
	struct Delegate : Noncopyable
	{
		struct Handle
		{
			friend Delegate<t_args...>;

			std::function<void(t_args...)> m_function;

			Handle() = default;
			
			~Handle()
			{
				if (m_delegate)
				{
					m_delegate->unbind(*this);
					m_delegate = nullptr;
				}
			}

			Handle(const Handle& in_other)
			{
				if (in_other.m_delegate)
					in_other.m_delegate->bind(*this);

				m_function = in_other.m_function;
			}

			Handle(Handle&& in_other) noexcept
			{
				if (in_other.m_delegate)
				{
					in_other.m_delegate->bind(*this);
					in_other.m_delegate->unbind(in_other);
				}

				m_function = std::move(in_other.m_function);
			}

			Handle& operator=(Handle&& in_other)
			{
				if (m_delegate)
					m_delegate->unbind(*this);

				if (in_other.m_delegate)
				{
					in_other.m_delegate->bind(*this);
					in_other.m_delegate->unbind(in_other);
				}

				m_function = std::move(in_other.m_function);
			}

			Handle& operator=(const Handle& in_other)
			{
				if (m_delegate)
					m_delegate->unbind(*this);

				if (in_other.m_delegate)
					in_other.m_delegate->bind(*this);

				m_function = in_other.m_function;
			}

		private:
			Delegate<t_args...>* m_delegate = nullptr;
		};

		Delegate() = default;
		Delegate(Delegate&& in_other) noexcept
		{
			m_handles = std::move(in_other.m_handles);
			for (Handle* it : m_handles)
			{
				if (it)
					it->m_delegate = this;
			}
		}

		Delegate& operator=(Delegate&& in_other) noexcept
		{
			m_handles = std::move(in_other.m_handles);
			for (Handle* it : m_handles)
			{
				if (it)
					it->m_delegate = this;
			}
		}

		virtual ~Delegate()
		{
			for (Handle* it : m_handles)
			{
				if (it)
					it->m_delegate = nullptr;
			}
		}

		void bind(Handle& in_out_handle)
		{
			std::scoped_lock lock(m_mutex);

			m_handles.push_back(&in_out_handle);
			in_out_handle.m_delegate = this;
		}

		void unbind(Handle& in_out_handle)
		{
			std::scoped_lock lock(m_mutex);

			for (size_t idx = 0; idx < m_handles.size(); idx++)
			{
				if (m_handles[idx] == &in_out_handle)
				{
					m_handles.erase(m_handles.begin() + idx);
					break;
				}
			}

			in_out_handle.m_delegate = nullptr;
		}

		void invoke(std::tuple<t_args...> in_params)
		{
			std::scoped_lock lock(m_mutex);

			for (Handle* it : m_handles)
			{
				if (it)
					std::apply(it->m_function, in_params);
			}
		}

		void invoke()
		{
			std::scoped_lock lock(m_mutex);

			for (Handle* it : m_handles)
			{
				if (it)
					it->m_function();
			}
		}

	private:
		std::vector<Handle*> m_handles;
		std::mutex m_mutex;
	};
}
#pragma once
#include <vector>
#include <functional>
#include <mutex>

namespace impuls
{
	template <typename ...t_args>
	struct delegate
	{
		struct handle
		{
			friend delegate<t_args...>;

			std::function<void(t_args...)> m_function;

			handle() = default;

			handle(const handle& in_other)
			{
				if (in_other.m_delegate)
					in_other.m_delegate->bind(*this);

				m_function = in_other.m_function;
			}

			handle(handle&& in_other)
			{
				if (in_other.m_delegate)
				{
					in_other.m_delegate->bind(*this);
					in_other.m_delegate->unbind(in_other);
				}

				m_function = std::move(in_other.m_function);
			}

			handle& operator=(const handle& in_other)
			{
				if (in_other.m_delegate)
					in_other.m_delegate->bind(*this);

				m_function = in_other.m_function;
			}

			~handle()
			{
				if (m_delegate)
				{
					m_delegate->unbind(*this);
					m_delegate = nullptr;
				}
			}

		private:
			delegate<t_args...>* m_delegate = nullptr;
		};

		delegate() = default;
		delegate(const delegate& in_other)
		{
			assert(in_other.m_handles.empty() && "copying delegate with valid handles is not accepted!");
		}

		delegate(delegate&& in_other)
		{
			m_handles = std::move(in_other.m_handles);
			for (handle* it : m_handles)
			{
				if (it)
					it->m_delegate = this;
			}
		}

		virtual ~delegate()
		{
			for (handle* it : m_handles)
			{
				if (it)
					it->m_delegate = nullptr;
			}
		}

		void bind(handle& in_out_handle)
		{
			std::scoped_lock lock(m_mutex);

			m_handles.push_back(&in_out_handle);
			in_out_handle.m_delegate = this;
		}

		void unbind(handle& in_out_handle)
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

			for (handle* it : m_handles)
			{
				if (it)
					std::apply(it->m_function, in_params);
			}
		}

	private:
		std::vector<handle*> m_handles;
		std::mutex m_mutex;
	};
}
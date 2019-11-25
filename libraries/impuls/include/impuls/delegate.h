#pragma once
#include <vector>
#include <functional>

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

		~delegate()
		{
			for (handle* it : m_handles)
			{
				if (it)
					it->m_delegate = nullptr;
			}
		}

		void bind(handle& in_out_handle)
		{
			m_handles.push_back(&in_out_handle);
			in_out_handle.m_delegate = this;
		}

		void unbind(handle& in_out_handle)
		{
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
			for (handle* it : m_handles)
			{
				if (it)
					std::apply(it->m_function, in_params);
			}
		}

	private:
		std::vector<handle*> m_handles;
	};
}
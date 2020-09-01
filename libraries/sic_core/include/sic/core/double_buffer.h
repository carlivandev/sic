#pragma once
#include <functional>
#include <mutex>

namespace sic
{
	template <typename T_buffer_type>
	struct Double_buffer
	{
		using Read_function = std::function<void(const T_buffer_type& in_read)>;
		using Write_function = std::function<void(T_buffer_type& in_write)> ;
		using Swap_function = std::function<void(T_buffer_type& in_read, T_buffer_type& in_write)>;

		void read(Read_function&& in_function)
		{
			std::scoped_lock lock(m_swap_mutex);
			in_function(*m_buffer_read);
		}
		
		void write(Write_function&& in_function)
		{
			std::scoped_lock lock_swap(m_swap_mutex);
			std::scoped_lock lock_write(m_write_mutex);
			in_function(*m_buffer_write);
		}

		void swap(Swap_function&& in_pre_swap_function)
		{
			std::scoped_lock lock_swap(m_swap_mutex);
			std::scoped_lock lock_write(m_write_mutex);

			in_pre_swap_function(*m_buffer_read, *m_buffer_write);

			if (m_buffer_read == &m_buffer_0)
			{
				m_buffer_read = &m_buffer_1;
				m_buffer_write = &m_buffer_0;
			}
			else
			{
				m_buffer_read = &m_buffer_0;
				m_buffer_write = &m_buffer_1;
			}
		}

	protected:
		T_buffer_type m_buffer_0;
		T_buffer_type m_buffer_1;

		T_buffer_type* m_buffer_read = &m_buffer_0;
		T_buffer_type* m_buffer_write = &m_buffer_1;

		std::mutex m_swap_mutex;
		std::mutex m_write_mutex;
	};
}
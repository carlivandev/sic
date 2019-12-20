#pragma once
#include <functional>
#include <mutex>

namespace impuls
{
	template <typename t_buffer_type>
	struct Double_buffer
	{
		typedef std::function<void(const t_buffer_type& in_read)> Read_function;
		typedef std::function<void(t_buffer_type& in_write)> Write_function;
		typedef std::function<void(t_buffer_type& in_read, t_buffer_type& in_write)> Swap_function;

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
		t_buffer_type m_buffer_0;
		t_buffer_type m_buffer_1;

		t_buffer_type* m_buffer_read = &m_buffer_0;
		t_buffer_type* m_buffer_write = &m_buffer_1;

		std::mutex m_swap_mutex;
		std::mutex m_write_mutex;
	};
}
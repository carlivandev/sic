#pragma once
#include <functional>
#include <mutex>

namespace impuls
{
	template <typename t_buffer_type>
	struct double_buffer
	{
		typedef std::function<void(const t_buffer_type& in_read)> read_function;
		typedef std::function<void(t_buffer_type& in_write)> write_function;
		typedef std::function<void(t_buffer_type& in_read, t_buffer_type& in_write)> swap_function;

		void read(read_function&& in_function)
		{
			std::scoped_lock lock(m_swap_mutex);
			in_function(*m_buffer_read);
		}
		
		void write(write_function&& in_function)
		{
			std::scoped_lock lock_swap(m_swap_mutex);
			std::scoped_lock lock_write(m_write_mutex);
			in_function(*m_buffer_write);
		}

		void swap(swap_function&& in_function)
		{
			std::scoped_lock lock_swap(m_swap_mutex);
			std::scoped_lock lock_write(m_write_mutex);

			in_function(*m_buffer_read, *m_buffer_write);

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
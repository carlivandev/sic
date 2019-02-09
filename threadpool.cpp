#include "stdafx.h"
#include "threadpool.h"
#include "thread_naming.h"

impuls::threadpool::~threadpool()
{
	shutdown();
}

void impuls::threadpool::spawn(ui16 in_worker_count)
{
	assert(is_caller_owner());

	for (ui16 i = 0; i < in_worker_count; ++i)
	{
		m_threads.emplace_back
		(
			[this]() -> void
		{
			closure task;

			std::unique_lock lock(m_mutex);

			while (!m_stop)
			{
				if (!m_tasks.empty())
				{
					task = std::move(m_tasks.back());
					m_tasks.pop_back();

					// execute the task
					lock.unlock();
					task();
					lock.lock();
				}
				else
				{
					while (m_tasks.empty() && !m_stop)
					{
						m_worker_signal.wait(lock);
					}
				}

			}
		}
		);

		set_thread_name(&m_threads.back(), ("worker_thread: " + std::to_string(i)).c_str());
	}
}

void impuls::threadpool::shutdown()
{
	assert(is_caller_owner());

	{
		std::scoped_lock lock(m_mutex);
		m_stop = true;
		m_worker_signal.notify_all();
	}

	for (auto& t : m_threads)
	{
		t.join();
	}

	m_threads.clear();
	m_stop = false;
}


void impuls::threadpool::emplace(closure&& in_closure)
{
	if (num_workers() == 0)
	{
		in_closure();
		return;
	}

	{
		std::scoped_lock lock(m_mutex);
		m_tasks.emplace_back(in_closure);
		m_worker_signal.notify_one();
	}
}

void impuls::threadpool::batch(std::vector<closure>&& in_closures)
{
	if (num_workers() == 0)
	{
		for (auto& t : in_closures)
			t();

		return;
	}

	const bool notify_all = in_closures.size() > 1;

	{
		std::scoped_lock lock(m_mutex);
		m_tasks.reserve(m_tasks.size() + in_closures.size());
		std::move(in_closures.begin(), in_closures.end(), std::back_inserter(m_tasks));
	}

	if (notify_all)
		m_worker_signal.notify_all();
	else
		m_worker_signal.notify_one();
}

bool impuls::threadpool::is_caller_owner() const
{
	return std::this_thread::get_id() == m_owner;
}

impuls::ui16 impuls::threadpool::num_workers() const
{
	return static_cast<ui16>(m_threads.size());
}

impuls::ui16 impuls::threadpool::num_tasks() const
{
	return static_cast<ui16>(m_tasks.size());
}
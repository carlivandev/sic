#include "impuls/pch.h"
#include "impuls/threadpool.h"
#include "impuls/thread_naming.h"

sic::Threadpool::~Threadpool()
{
	shutdown();
}

void sic::Threadpool::spawn(ui16 in_worker_count)
{
	assert(is_caller_owner());

	for (ui16 i = 0; i < in_worker_count; ++i)
		m_threads_initialized.push_back(false);

	for (ui16 i = 0; i < in_worker_count; ++i)
	{
		m_threads.emplace_back
		(
			[this, i]() -> void
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
						m_threads_initialized[i] = true;
						m_worker_signal.wait(lock);
					}
				}

			}
		}
		);

		const std::string name = ("worker_thread: " + std::to_string(i));
		m_thread_id_to_name[m_threads.back().get_id()] = name;
		set_thread_name(&m_threads.back(), name.c_str());
	}

	bool ready = false;

	while (!ready)
	{
		ready = true;

		for (ui16 i = 0; i < in_worker_count; ++i)
			if (!m_threads_initialized[i])
				ready = false;
	}
}

void sic::Threadpool::shutdown()
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


void sic::Threadpool::emplace(closure&& in_closure)
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

void sic::Threadpool::batch(std::vector<closure>&& in_closures)
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

bool sic::Threadpool::is_caller_owner() const
{
	return std::this_thread::get_id() == m_owner;
}

sic::ui16 sic::Threadpool::num_workers() const
{
	return static_cast<ui16>(m_threads.size());
}

sic::ui16 sic::Threadpool::num_tasks() const
{
	return static_cast<ui16>(m_tasks.size());
}

const char* sic::Threadpool::thread_name(std::thread::id in_id) const
{
	auto it = m_thread_id_to_name.find(in_id);

	if (it != m_thread_id_to_name.end())
		return it->second.c_str();

	return nullptr;
}

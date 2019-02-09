#pragma once
#include <mutex>

namespace impuls
{
	class threadpool
	{
	public:

		typedef std::function<void()> closure;

		threadpool() = default;
		~threadpool();

		void spawn(ui16 in_worker_count);
		void shutdown();

		void emplace(closure&& in_closure);

		void batch(std::vector<closure>&& in_closures);

		bool is_caller_owner() const;

		ui16 num_workers() const;
		ui16 num_tasks() const;

	private:
		const std::thread::id m_owner{ std::this_thread::get_id() };

		mutable std::mutex m_mutex;

		std::condition_variable m_worker_signal;
		std::vector<closure> m_tasks;
		std::vector<std::thread> m_threads;

		bool m_stop = false;
	};
};
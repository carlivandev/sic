#pragma once
#include "sic/defines.h"
#include "sic/type_restrictions.h"

#include <mutex>
#include <unordered_map>
#include <functional>

namespace sic
{
	struct Threadpool : Noncopyable
	{
	public:
		using Closure = std::function<void()>;

		Threadpool() = default;
		~Threadpool();

		void spawn(ui16 in_worker_count);
		void shutdown();

		void emplace(Closure&& in_closure);

		void batch(std::vector<Closure>&& in_closures);

		bool is_caller_owner() const;
		bool is_shutting_down() const { return m_stop; }

		ui16 num_workers() const;
		ui16 num_tasks() const;

		const char* thread_name(std::thread::id in_id) const;

	private:
		const std::thread::id m_owner{ std::this_thread::get_id() };

		mutable std::mutex m_mutex;

		std::condition_variable m_worker_signal;
		std::vector<Closure> m_tasks;
		std::vector<std::thread> m_threads;
		std::vector<bool> m_threads_initialized;
		std::unordered_map<std::thread::id, std::string> m_thread_id_to_name;
		bool m_stop = false;
	};
};
#pragma once
#include "defines.h"

#include <functional>
#include <queue>
#include <mutex>
#include <optional>

namespace sic
{
	struct Job_id
	{
		struct Dependency
		{
			i32 m_submitted_on_thread_id = -1;
			i32 m_id = -1;
		};

		i32 m_submitted_on_thread_id = -1;
		i32 m_id = -1;
		std::optional<Dependency> m_job_dependency;
		bool m_run_on_main_thread = false;
	};

	struct Job_node;
	struct Threadpool;

	struct Main_thread_worker
	{
		void try_do_job();

		void push_back_job(Job_node* in_node);

		std::queue<Job_node*> m_jobs;
		std::mutex m_job_list_mutex;
	};

	struct Job_node
	{
		void do_job();

		std::function<void()> m_job;
		std::vector<Job_node*> m_depends_on_me;
		std::mutex m_mutex;
		i32 m_dependencies_left = 0;
		bool m_is_ready_to_execute = false;
		bool m_job_finished = false;
		bool m_run_on_main_thread = false;
		bool m_profiling = false;
		Main_thread_worker* m_main_thread_worker = nullptr;
		Threadpool* m_threadpool = nullptr;

		Job_id m_id;
	};
}
#include "sic/core/engine_job_scheduling.h"

#include "sic/core/threadpool.h"

void sic::Main_thread_worker::try_do_job()
{
	if (!m_jobs.empty())
	{
		Job_node* job_to_execute;
		{
			std::scoped_lock lock(m_job_list_mutex);
			job_to_execute = m_jobs.front();
			m_jobs.pop();
		}
		job_to_execute->do_job();
	}
}

void sic::Main_thread_worker::push_back_job(Job_node* in_node)
{
	std::scoped_lock lock(m_job_list_mutex);
	m_jobs.push(in_node);
}

void sic::Job_node::do_job()
{
	m_job();
	m_job_finished = true;

	for (i32 i = m_depends_on_me.size() - 1; i >= 0; i--)
	{
		Job_node* depend_on_me = m_depends_on_me[i];

		std::scoped_lock lock(depend_on_me->m_mutex);
		--depend_on_me->m_dependencies_left;

		if (depend_on_me->m_dependencies_left <= 0)
		{
			m_depends_on_me.pop_back();

			depend_on_me->m_is_ready_to_execute = true;

			if (depend_on_me->m_run_on_main_thread)
			{
				m_main_thread_worker->push_back_job(depend_on_me);
			}
			else
			{
				if (m_depends_on_me.size() == 0)
					depend_on_me->do_job();
				else
					m_threadpool->emplace([depend_on_me]() { depend_on_me->do_job(); });
			}
		}
	}
}

#include "sic/core/thread_context.h"
#include "sic/core/scene_context.h"

sic::Log sic::g_log_temporary_storage("Temporary_storage", false);

sic::Thread_context& sic::this_thread()
{
	static thread_local Thread_context context;
	return context;
}

size_t sic::Thread_context::add_query()
{
	if (m_free_query_indices.empty())
	{
		m_queries.emplace_back();
		return m_queries.size() - 1;
	}

	const size_t new_idx = m_free_query_indices.back();
	m_free_query_indices.pop_back();
	m_queries[new_idx] = nullptr;

	return new_idx;
}

void sic::Thread_context::trigger_query(Thread_context* in_thread, size_t in_id)
{
	m_queries_to_trigger.emplace_back(in_thread, in_id);
}

void sic::Thread_context::set_query(size_t in_id, std::function<void(Engine_processor<>)> in_query_job)
{
	m_queries[in_id] = in_query_job;
}

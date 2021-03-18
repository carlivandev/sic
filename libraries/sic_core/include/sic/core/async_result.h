#pragma once
#include "sic/core/scene_context.h"

namespace sic
{
	struct Async_query
	{
		//once setup, trigger should always be called or else the query will never be removed
		void setup()
		{
			m_thread_context = &this_thread();
			m_id = m_thread_context->add_query();
		}

		void trigger()
		{
			if (!m_thread_context)
				return;

			if (!m_id.has_value())
				return;

			this_thread().trigger_query(m_thread_context, m_id.value());
		}

		//if id is bound, lookup on thread
		template <typename T_job>
		Async_query then(Engine_processor<> in_processor, T_job in_job)
		{
			Async_query next_query;
			next_query.setup();
			
			if (!m_thread_context)
			{
				schedule_immediate(in_processor, std::function(in_job), next_query);
			}
			else
			{
				m_thread_context->set_query(m_id.value(), [in_job, next_query](Engine_processor<> in_processor) mutable { in_processor.schedule(in_job); next_query.trigger(); });
			}

			return next_query;
		}

		bool is_setup() const { return m_id.has_value(); }

	private:
		template <typename ...T_args>
		void schedule_immediate(Engine_processor<> in_processor, std::function<void(sic::Ep<T_args...>)> in_job, const Async_query& in_query)
		{
			in_processor.schedule([in_job, query = in_query](sic::Ep<T_args...> in_processor) mutable { in_job(in_processor); query.trigger(); });
		}


		Thread_context* m_thread_context = nullptr;
		std::optional<i32> m_id;
	};
}
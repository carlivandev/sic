#pragma once
#include "sic/core/thread_temporary_storage_instance.h"
#include "sic/core/logger.h"
#include "sic/core/engine_job_scheduling.h"

#include <vector>
#include <string>
#include <functional>

namespace sic
{
	extern Log g_log_temporary_storage;

	struct Engine_context;

	struct Thread_context
	{
		friend Thread_temporary_storage_instance;
		friend struct Engine;
		friend struct Engine_context;

		constexpr static size_t storage_byte_size = 32768 * 2;

		~Thread_context()
		{
			delete[] m_storage;
		}

		Thread_temporary_storage_instance allocate_temporary_storage(size_t in_bytes_to_allocate)
		{
			if (m_current + in_bytes_to_allocate > m_storage + storage_byte_size)
			{
				SIC_LOG_W(g_log_temporary_storage, "({0}) Storage full, heap allocated: {1} bytes", m_name, in_bytes_to_allocate);

				m_dynamically_allocated_blocks.push_back(new char[in_bytes_to_allocate]);
				return Thread_temporary_storage_instance(m_dynamically_allocated_blocks.back(), in_bytes_to_allocate, *this);
			}

			char* ret_val = m_current;

			m_current += in_bytes_to_allocate;

			SIC_LOG(g_log_temporary_storage, "({0}) Allocated: {1} bytes", m_name, in_bytes_to_allocate);
			print_temporary_storage_status();

			return Thread_temporary_storage_instance(ret_val, in_bytes_to_allocate, *this);
		}

		void set_current_offset(char* in_new_offset)
		{
			m_current = in_new_offset;
			SIC_LOG(g_log_temporary_storage, "({0}) Moved offset to: {1}", m_name, m_current - m_storage);
			print_temporary_storage_status();

		}
		char* get_current_offset() const
		{
			return m_current;
		}

		void reset_local_storage()
		{
			m_current = m_storage;

			for (char* block : m_dynamically_allocated_blocks)
				delete[] block;

			m_dynamically_allocated_blocks.clear();

			SIC_LOG(g_log_temporary_storage, "({0}) Storage reset", m_name);
			print_temporary_storage_status();
		}

		void set_name(const std::string& in_name)
		{
			m_name = in_name;
		}

		void update_deferred(std::function<void(Engine_context)> in_func)
		{
			m_deferred_updates.push_back(in_func);
		}

		const std::string& get_name() const { return m_name; }
		i32 get_id() const { return m_id; }

	private:
		void print_temporary_storage_status()
		{
			SIC_LOG
			(
				g_log_temporary_storage,
				"({0}) Status: {1}/{2} Bytes ({3}%%)",
				m_name,
				m_current - m_storage,
				storage_byte_size,
				static_cast<i32>((static_cast<float>(m_current - m_storage) / static_cast<float>(storage_byte_size)) * 100)
			);
		}

		char* m_storage = new char[storage_byte_size];
		char* m_current = m_storage;

		size_t storage_refrence_count = 0;

		std::vector<char*> m_dynamically_allocated_blocks;

		std::vector<std::function<void(Engine_context)>> m_deferred_updates;
		std::vector<std::pair<Job_id, std::function<void(Engine_context, Job_id)>>> m_scheduled_items;

		struct Timed_scheduled_item
		{
			std::function<void(Engine_context)> m_func;
			float m_time_left = 0.0f;
			float m_time_until_start_scheduling = 0.0f;
			sic::i64 m_frame_when_ticked = 0;
		};
		std::vector<Timed_scheduled_item> m_timed_scheduled_items;
		std::vector<size_t> m_free_timed_scheduled_items_indices;

		size_t m_job_id_ticker = 0;
		size_t m_job_index_offset = 0;

		std::string m_name;
		i32 m_id = -1;
	};

	Thread_context& this_thread();


	struct Scoped_temporary_storage_reset
	{
		Scoped_temporary_storage_reset() : m_context(this_thread()), m_offset_to_reset_to(m_context.get_current_offset())
		{
		}

		~Scoped_temporary_storage_reset()
		{
			m_context.set_current_offset(m_offset_to_reset_to);
		}

		Thread_context& m_context;
		char* m_offset_to_reset_to;
	};
}
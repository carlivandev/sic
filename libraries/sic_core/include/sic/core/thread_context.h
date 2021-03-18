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

	template <typename ...T>
	struct Engine_processor;

	struct Schedule_timed_handle
	{
		friend struct Engine_context;
		friend struct Scene_context;

		i32 get_thread_id() const { return m_thread_id; }
		size_t get_job_index() const { return m_job_index; }
		size_t get_unique_id() const { return m_unique_id; }

	private:
		i32 m_thread_id = -1;
		size_t m_job_index = 0;
		size_t m_unique_id = 0;
	};

	struct Thread_context
	{
		friend Thread_temporary_storage_instance;
		friend struct Engine;
		friend struct Engine_context;
		friend struct Scene_context;

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

		void profile_start(const char* in_key)
		{
			Profiling_data::Item new_item;
			new_item.m_start = std::chrono::high_resolution_clock::now();
			new_item.m_key = in_key;
			m_profiling_data.m_profile_stack.push_back(new_item);
		}

		void profile_end()
		{
			auto& back = m_profiling_data.m_profile_stack.back();
			back.m_end = std::chrono::high_resolution_clock::now();

			m_profiling_data.m_items.push_back(back);
			m_profiling_data.m_profile_stack.pop_back();
		}

		size_t add_query();
		void trigger_query(Thread_context* in_thread, size_t in_id);
		void set_query(size_t in_id, std::function<void(Engine_processor<>)> in_query_job);

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
			std::function<bool(Engine_context)> m_unschedule_callback;
			std::function<void()> m_job_func;
			float m_time_left = 0.0f;
			float m_time_until_start_scheduling = 0.0f;
			sic::i64 m_frame_when_ticked = 0;
			size_t m_unique_id = 0;
			std::optional<i32> m_scene_id;
		};
		std::vector<Timed_scheduled_item> m_timed_scheduled_items;
		std::vector<size_t> m_free_timed_scheduled_items_indices;
		std::vector<Schedule_timed_handle> m_scheduled_items_to_remove;

		i32 m_job_id_ticker = 0;
		size_t m_job_index_offset = 0;

		size_t m_timed_schedule_handle_id_ticker = 0;

		std::vector<std::function<void(Engine_processor<>)>> m_queries;
		std::vector<size_t> m_free_query_indices;
		std::vector<std::pair<Thread_context*, size_t>> m_queries_to_trigger;

		std::string m_name;
		i32 m_id = -1;

		struct Profiling_data
		{
			struct Item
			{
				std::chrono::steady_clock::time_point m_start;
				std::chrono::steady_clock::time_point m_end;
				std::string m_key;
			};

			std::vector<Item> m_profile_stack;
			std::vector<Item> m_items;
		} m_profiling_data;
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
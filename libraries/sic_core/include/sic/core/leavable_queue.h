#pragma once
#include "sic/core/defines.h"

#include <vector>
#include <assert.h>
#include <mutex>

namespace sic
{
	/*
		threadsafe queue, that uses a ticket system to allow mid-queue leaving
	*/
	template <typename t>
	struct Leavable_queue
	{
		struct Queue_ticket
		{
			friend Leavable_queue<t>;

			Queue_ticket() = default;
			Queue_ticket(size_t in_ticket_index, i32 in_ticket_id) : m_ticket_index(in_ticket_index), m_ticket_id(in_ticket_id) {}

			size_t ticket_index() const { return m_ticket_index; }
		private:
			size_t m_ticket_index = 0;
			i32 m_ticket_id = 0;
		};

		struct Queue_item
		{
			void reset()
			{
				m_next = nullptr;
				m_previous = nullptr;
				m_index = -1;
			}

			Queue_item* m_next = nullptr;
			Queue_item* m_previous = nullptr;

			t m_item = {};
			i32 m_index = -1;
			i32 m_ticket_id = 0;
		};

		Leavable_queue() = default;
		Leavable_queue(size_t in_size)
		{
			set_size(in_size);
		}

		void set_size(size_t in_size)
		{
			std::scoped_lock lock(m_mutex);

			assert(m_buffer.empty() && "size has already been set");

			m_buffer.resize(in_size);
			m_free_indices.reserve(in_size);

			for (i32 i = static_cast<i32>(in_size) - 1; i >= 0; i--)
				m_free_indices.push_back(i);
		}

		bool is_queue_full() const { return m_free_indices.empty(); }
		bool is_queue_empty() const { return !m_first; }

		Queue_ticket enqueue(t in_item)
		{
			std::scoped_lock lock(m_mutex);

			assert(!is_queue_full() && "queue is full!");

			const size_t idx = m_free_indices.back();
			m_free_indices.pop_back();

			Queue_item& new_item = m_buffer[idx];
			new_item.m_index = static_cast<i32>(idx);
			new_item.m_ticket_id = m_ticket_id_ticker++;
			new_item.m_item = in_item;

			if (!m_first)
				m_first = &new_item;

			if (m_last)
			{
				m_last->m_previous = &new_item;
				new_item.m_next = m_last;
			}

			m_last = &new_item;

			return Queue_ticket(idx, new_item.m_ticket_id);
		}

		t dequeue()
		{
			std::scoped_lock lock(m_mutex);

			assert(!is_queue_empty() && "queue is empty!");

			Queue_item* dequeued = m_first;
			m_free_indices.push_back(dequeued->m_index);

			if (dequeued == m_last)
				m_last = nullptr;
			else
				dequeued->m_previous->m_next = nullptr;

			m_first = dequeued->m_previous;

			t to_return;
			std::swap(to_return, dequeued->m_item);

			dequeued->reset();

			return std::move(to_return);
		}

		void leave_queue(Queue_ticket in_ticket)
		{
			std::scoped_lock lock(m_mutex);

			Queue_item& leaver = m_buffer[in_ticket.m_ticket_index];
			
			//ticket index invalid, item has already left queue
			if (leaver.m_index == -1)
				return;

			//ticket id invalid, item has already left queue
			if (leaver.m_ticket_id != leaver.m_ticket_id)
				return;

			if (&leaver == m_first)
				m_first = leaver.m_previous;
			else
				leaver.m_next->m_previous = leaver.m_previous;

			if (&leaver == m_last)
				m_last = leaver.m_next;
			else
				leaver.m_previous->m_next = leaver.m_next;

			m_free_indices.push_back(leaver.m_index);
			leaver.reset();
		}

	private:
		std::vector<Queue_item> m_buffer;
		std::vector<size_t> m_free_indices;
		i32 m_ticket_id_ticker = 0;

		Queue_item * m_first = nullptr;
		Queue_item * m_last = nullptr;

		std::mutex m_mutex;
	};
}
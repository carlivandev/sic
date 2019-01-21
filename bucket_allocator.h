#pragma once
#include "pch.h"

#include <vector>

namespace impuls
{
	struct bucket_byte_allocator
	{
		struct bucket
		{
			void deallocate()
			{
				if (m_data)
				{
					delete[] m_data;
					m_data = nullptr;
				}
			}
			void allocate(ui32 in_capacity)
			{
				deallocate();
				m_data = new byte[in_capacity];
				memset(m_data, 0, in_capacity);
			}
			byte* m_data = nullptr;
		};

		~bucket_byte_allocator()
		{
			deallocate();
		}

		void allocate(ui32 in_initial_capacity, ui32 in_bucket_capacity)
		{
			m_initial_bucket.allocate(in_initial_capacity);
			m_initial_capacity = in_initial_capacity;
			m_bucket_size = in_bucket_capacity;
		}

		void deallocate()
		{
			for (auto& bucket : m_buckets)
			{
				bucket.deallocate();
			}
			m_buckets.clear();

			m_initial_bucket.deallocate();

			m_initial_capacity = 0;
			m_bucket_size = 0;
			m_current_size = 0;
		}

		byte& emplace_back(ui32 in_byte_count)
		{
			assert(m_bucket_size >= in_byte_count && "");

			const ui32 at = m_current_size;
			m_current_size += in_byte_count;

			if (m_current_size <= m_initial_capacity)
			{
				return m_initial_bucket.m_data[at];
			}
			else
			{
				const ui32 pos = at - m_initial_capacity;
				const ui32 bucket_idx = pos / m_bucket_size;
				const ui32 pos_in_bucket = pos - (bucket_idx * m_bucket_size);


				if (bucket_idx == m_buckets.size())
				{
					m_buckets.emplace_back();
					m_buckets.back().allocate(m_bucket_size);
				}

				return m_buckets.back().m_data[pos_in_bucket];
			}
		}

		byte& operator[] (ui32 in_index)
		{
			const ui32 pos = in_index - m_initial_capacity;
			const ui32 bucket_idx = pos / m_bucket_size;
			const ui32 pos_in_bucket = pos - (bucket_idx * m_bucket_size);

			if (in_index < m_initial_capacity)
				return m_initial_bucket.m_data[in_index];

			return m_buckets[bucket_idx].m_data[pos_in_bucket];
		}

		const byte& operator[] (ui32 in_index) const
		{
			const ui32 pos = in_index - m_initial_capacity;
			const ui32 bucket_idx = pos / m_bucket_size;
			const ui32 pos_in_bucket = pos - (bucket_idx * m_bucket_size);

			if (in_index < m_initial_capacity)
				return m_initial_bucket.m_data[in_index];

			return m_buckets[bucket_idx].m_data[pos_in_bucket];
		}

		ui32 size() const
		{
			return m_current_size;
		}

		std::vector<bucket> m_buckets;
		bucket m_initial_bucket;
		ui32 m_initial_capacity = 0;
		ui32 m_bucket_size = 0;
		ui32 m_current_size = 0;
	};

	template <typename element_type>
	struct bucket_allocator
	{
		void allocate(ui32 in_initial_capacity, ui32 in_bucket_capacity)
		{
			m_byte_allocator.allocate(in_initial_capacity * sizeof(element_type), in_bucket_capacity * sizeof(element_type));
		}

		element_type& emplace_back()
		{
			byte& pos = m_byte_allocator.emplace_back(sizeof(element_type));
			element_type* new_element = reinterpret_cast<element_type*>(&pos);

			new (new_element) element_type();
			return *new_element;
		}

		element_type& operator[] (ui32 in_index)
		{
			return *reinterpret_cast<element_type*>(&m_byte_allocator[in_index * sizeof(element_type)]);
		}

		const byte& operator[] (ui32 in_index) const
		{
			return *reinterpret_cast<element_type*>(&m_byte_allocator[in_index * sizeof(element_type)]);
		}

		ui32 size() const
		{
			return m_byte_allocator.size() / sizeof(element_type);
		}

		void deallocate()
		{
			m_byte_allocator.deallocate();
		}

		bucket_byte_allocator m_byte_allocator;
	};
}
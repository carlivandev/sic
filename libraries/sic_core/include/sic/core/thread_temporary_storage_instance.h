#pragma once

namespace sic
{
	struct Thread_context;

	struct Thread_temporary_storage_instance
	{
		Thread_temporary_storage_instance() = delete;
		Thread_temporary_storage_instance(const Thread_temporary_storage_instance& in_other) = delete;
		Thread_temporary_storage_instance& operator=(const Thread_temporary_storage_instance& in_other) = delete;

		Thread_temporary_storage_instance(char* in_storage, size_t in_byte_size, Thread_context& in_context);

		Thread_temporary_storage_instance(Thread_temporary_storage_instance&& in_other) noexcept;
		Thread_temporary_storage_instance& operator=(Thread_temporary_storage_instance&& in_other) noexcept;

		char* get_storage() const { return m_storage; }
		size_t get_byte_size() const { return m_byte_size; }

	private:
		char* m_storage;
		size_t m_byte_size;
		Thread_context& m_context;
	};
}

#include "sic/core/thread_temporary_storage_instance.h"
#include "sic/core/thread_context.h"

sic::Thread_temporary_storage_instance::Thread_temporary_storage_instance(char* in_storage, size_t in_byte_size, Thread_context& in_context)
	: m_storage(in_storage), m_byte_size(in_byte_size), m_context(in_context)
{
}

sic::Thread_temporary_storage_instance::Thread_temporary_storage_instance(Thread_temporary_storage_instance&& in_other) noexcept
	: m_storage(in_other.m_storage), m_byte_size(in_other.m_byte_size), m_context(in_other.m_context)
{
	in_other.m_storage = nullptr;
}

sic::Thread_temporary_storage_instance& sic::Thread_temporary_storage_instance::operator=(Thread_temporary_storage_instance&& in_other) noexcept
{
	m_storage = in_other.m_storage;
	m_byte_size = in_other.m_byte_size;
	m_context = in_other.m_context;

	in_other.m_storage = nullptr;
	return *this;
}

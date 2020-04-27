#include "sic/temporary_string.h"

sic::Temporary_string::Temporary_string() : m_storage(sic::this_thread().allocate_temporary_storage(1))
{
	memcpy(m_storage.get_storage(), "/0", 1);
}

sic::Temporary_string::Temporary_string(const char* in_string, size_t in_length) : m_storage(sic::this_thread().allocate_temporary_storage(in_length))
{
	memcpy(m_storage.get_storage(), in_string, in_length);
}

sic::Temporary_string::Temporary_string(const char* in_string) : m_storage(sic::this_thread().allocate_temporary_storage(strlen(in_string) + 1))
{
	memcpy(m_storage.get_storage(), in_string, m_storage.get_byte_size());
}

sic::Temporary_string::Temporary_string(const Temporary_string& in_other) : m_storage(this_thread().allocate_temporary_storage(in_other.m_storage.get_byte_size()))
{
	memcpy(m_storage.get_storage(), in_other.m_storage.get_storage(), m_storage.get_byte_size());
}

std::optional<size_t> sic::Temporary_string::find_char_from_start(char in_char, size_t in_offset) const
{
	char* ptr = strchr(m_storage.get_storage() + in_offset, in_char);

	if (ptr)
		return ptr - m_storage.get_storage();

	return {};
}

std::optional<size_t> sic::Temporary_string::find_string_from_start(const char* in_string, size_t in_offset) const
{
	char* ptr = strstr(m_storage.get_storage() + in_offset, in_string);

	if (ptr)
		return ptr - m_storage.get_storage();

	return {};
}

char sic::Temporary_string::at(size_t in_index) const
{
	assert(in_index < get_length() && "Index out of bounds!");
	return m_storage.get_storage()[in_index];
}

bool sic::Temporary_string::operator==(const Temporary_string& in_other) const
{
	return strcmp(get_c_str(), in_other.get_c_str()) == 0;
}

bool sic::Temporary_string::operator!=(const Temporary_string& in_other) const
{
	return strcmp(get_c_str(), in_other.get_c_str()) != 0;
}

sic::Temporary_string sic::Temporary_string::operator+(const Temporary_string& in_other) const
{
	const size_t result_length = in_other.get_length() + get_length();

	Temporary_string result(result_length);
	result.set_data(get_c_str(), 0, get_length());
	result.set_data(in_other.get_c_str(), get_length(), in_other.get_length());

	return result;
}

sic::Temporary_string sic::Temporary_string::operator+(const char* in_other) const
{
	const size_t other_length = strlen(in_other);
	const size_t result_length = other_length + get_length();

	Temporary_string result(result_length);
	result.set_data(get_c_str(), 0, get_length());
	result.set_data(in_other, get_length(), other_length);

	return result;
}

const sic::Temporary_string& sic::Temporary_string::operator=(const Temporary_string& in_other)
{
	m_storage = std::move(this_thread().allocate_temporary_storage(in_other.m_storage.get_byte_size()));
	memcpy(m_storage.get_storage(), in_other.m_storage.get_storage(), in_other.m_storage.get_byte_size());

	return *this;
}

const sic::Temporary_string& sic::Temporary_string::operator=(Temporary_string&& in_other) noexcept
{
	m_storage = std::move(in_other.m_storage);
	return *this;
}

sic::Temporary_string sic::Temporary_string::append(const Temporary_string& in_other, size_t in_b_offset, size_t in_b_length) const
{
	const size_t result_length = get_length() + in_b_length;

	Temporary_string result(result_length);
	result.set_data(get_c_str(), 0, get_length());
	result.set_data(in_other.get_c_str() + in_b_offset, get_length(), in_b_length);

	return result;
}

sic::Temporary_string sic::Temporary_string::append(const Temporary_string& in_other) const
{
	return append(in_other, 0, in_other.get_length());
}

sic::Temporary_string sic::Temporary_string::append_from_start(const Temporary_string& in_other, size_t in_other_length) const
{
	assert(in_other_length <= in_other.get_length() && "Length can not exceed the length of in_other.");
	return append(in_other, 0, in_other_length);
}

sic::Temporary_string sic::Temporary_string::append_from_end(const Temporary_string& in_other, size_t in_other_length) const
{
	assert(in_other_length <= in_other.get_length() && "Length can not exceed the length of in_other.");
	return append(in_other, in_other.get_length() - in_other_length, in_other_length);
}

sic::Temporary_string sic::Temporary_string::remove(size_t in_offset, size_t in_length) const
{
	assert(in_offset < get_length() && "Substring not in range of in_a.");
	assert(in_length + in_offset <= get_length() && "Substring not in range of in_a.");

	const size_t result_length = get_length() - in_length;

	Temporary_string result(result_length);
	result.set_data(get_c_str(), 0, in_offset);
	result.set_data(get_c_str() + in_offset + in_length, in_offset, result_length - in_offset);

	return result;
}

sic::Temporary_string sic::Temporary_string::substring(size_t in_offset, size_t in_length) const
{
	assert(in_offset < get_length() && "Substring not in range of in_a.");
	assert(in_length + in_offset <= get_length() && "Substring not in range of in_a.");

	Temporary_string result(in_length);
	result.set_data(get_c_str() + in_offset, 0, in_length);

	return result;
}

const char* sic::Temporary_string::begin() const
{
	return m_storage.get_storage();
}

const char* sic::Temporary_string::end() const
{
	return m_storage.get_storage() + get_length();
}

void sic::Temporary_string::set_data(const char* in_data, size_t in_byte_offset, size_t in_byte_size)
{
	memcpy(m_storage.get_storage() + in_byte_offset, in_data, in_byte_size);
}

#pragma once
#include "sic/thread_context.h"

#include <optional>
#include <cassert>
#include <string>

namespace sic
{
	struct Temporary_string
	{
		Temporary_string();
		Temporary_string(const char* in_string, size_t in_length);
		Temporary_string(const char* in_string);
		Temporary_string(const Temporary_string& in_other);
		Temporary_string(size_t in_length) : m_storage(sic::this_thread().allocate_temporary_storage(in_length + 1)) { memcpy(m_storage.get_storage() + in_length, "\0", 1); }

		size_t get_length() const { return m_storage.get_byte_size() - 1; }
		bool get_is_empty() const { return get_length() == 0; }
		const char* get_c_str() const { return m_storage.get_storage(); }

		char* get_data() { return m_storage.get_storage(); }

		std::optional<size_t> find_char_from_start(char in_char, size_t in_offset = 0) const;
		std::optional<size_t> find_string_from_start(const char* in_string, size_t in_offset = 0) const;

		char at(size_t in_index) const;

		bool operator==(const Temporary_string& in_other) const;
		bool operator!=(const Temporary_string& in_other) const;

		[[nodiscard]] Temporary_string operator+(const Temporary_string& in_other) const;
		[[nodiscard]] Temporary_string operator+(const char* in_other) const;

		const Temporary_string& operator=(const Temporary_string& in_other);
		const Temporary_string& operator=(Temporary_string&& in_other) noexcept;

		[[nodiscard]] Temporary_string append(const Temporary_string& in_other, size_t in_b_offset, size_t in_b_length) const;
		[[nodiscard]] Temporary_string append(const Temporary_string& in_other) const;
		[[nodiscard]] Temporary_string append_from_start(const Temporary_string& in_other, size_t in_other_length) const;
		[[nodiscard]] Temporary_string append_from_end(const Temporary_string& in_other, size_t in_other_length) const;

		[[nodiscard]] Temporary_string remove(size_t in_offset, size_t in_length) const;

		[[nodiscard]] Temporary_string substring(size_t in_offset, size_t in_length) const;

		const char* begin() const;
		const char* end() const;

	private:
		void set_data(const char* in_data, size_t in_byte_offset, size_t in_byte_size);

		Thread_temporary_storage_instance m_storage;
	};

	
}

namespace std
{
	template <>
	struct hash<sic::Temporary_string>
	{
		size_t operator()(const sic::Temporary_string& in_string) const
		{
			return hash<string_view>()(in_string.get_c_str());
		}
	};
}
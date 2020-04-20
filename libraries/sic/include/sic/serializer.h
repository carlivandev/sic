#pragma once
#include "defines.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <map>

namespace sic
{
	struct Serialize_stream
	{
		template <typename T_data_type>
		void write(const T_data_type& in)
		{
			serialize(in, *this);
		}

		template <typename T_data_type>
		void write_memcpy(const T_data_type& in)
		{
			serialize_memcpy(in, *this);
		}

		void write_raw(unsigned char* in_buffer, ui32 in_buffer_bytesize);

		std::string m_bytes;
	};

	struct Deserialize_stream
	{
		Deserialize_stream(std::string&& in_bytes) : m_bytes(in_bytes) {}

		template <typename T_data_type>
		void read(T_data_type& out)
		{
			deserialize(*this, out);
		}

		template <typename T_data_type>
		void read_memcpy(T_data_type& out)
		{
			deserialize_memcpy(*this, out);
		}

		void read_raw(unsigned char*& out_buffer, ui32& out_buffer_bytesize);

		std::string m_bytes;
		ui32 m_offset = 0;
	};

	void serialize_raw(unsigned char* in_buffer, ui32 in_buffer_bytesize, Serialize_stream& out_stream);

	void deserialize_raw(Deserialize_stream& in_stream, unsigned char*& out_buffer, ui32& out_buffer_bytesize);

	template <typename T_data_type>
	void serialize_memcpy(const T_data_type& in_to_serialize, Serialize_stream& out_stream)
	{
		const size_t old_size = out_stream.m_bytes.size();
		out_stream.m_bytes.resize(old_size + sizeof(T_data_type));
		memcpy_s(&out_stream.m_bytes[old_size], sizeof(T_data_type), &in_to_serialize, sizeof(T_data_type));
	}

	template <typename T_data_type>
	void deserialize_memcpy(Deserialize_stream & in_stream, T_data_type & out_deserialized)
	{
		memcpy_s(&out_deserialized, sizeof(T_data_type), &(in_stream.m_bytes[in_stream.m_offset]), sizeof(T_data_type));
		in_stream.m_offset += sizeof(T_data_type);
	}

	template <typename T_data_type>
	void serialize(const T_data_type& in_to_serialize, Serialize_stream& out_stream)
	{
		static_assert(std::is_trivially_copyable<T_data_type>::value, "Type is not trivially serializable! Please specialize serialize or use Serialize_stream::write_memcpy/write_raw.");
		serialize_memcpy(in_to_serialize, out_stream);
	}

	template <typename T_data_type>
	void deserialize(Deserialize_stream& in_stream, T_data_type& out_deserialized)
	{
		static_assert(std::is_trivially_copyable<T_data_type>::value, "Type is not trivially deserializable! Please specialize deserialize or use Deserialize_stream::read_memcpy/read_raw.");
		deserialize_memcpy(in_stream, out_deserialized);
	}

	template <typename T_data_type>
	void serialize(const std::vector<T_data_type>& in_to_serialize, Serialize_stream& out_stream)
	{
		serialize(in_to_serialize.size(), out_stream);

		for (const T_data_type& element : in_to_serialize)
			serialize(element, out_stream);
	}

	template <typename T_data_type>
	void deserialize(Deserialize_stream& in_stream, std::vector<T_data_type>& out_deserialized)
	{
		const size_t old_len = out_deserialized.size();

		size_t len = 0;
		deserialize(in_stream, len);
		
		out_deserialized.resize(old_len + len);

		for (size_t i = old_len; i < old_len + len; i++)
			deserialize(in_stream, out_deserialized[i]);
	}

	template <typename T_map>
	void serialize_map(const T_map& in_to_serialize, Serialize_stream& out_stream)
	{
		serialize(in_to_serialize.size(), out_stream);

		for (auto element : in_to_serialize)
		{
			serialize(element.first, out_stream);
			serialize(element.second, out_stream);
		}
	}

	template <typename T_key, typename T_value>
	void serialize(const std::map<T_key, T_value>& in_to_serialize, Serialize_stream& out_stream)
	{
		serialize_map(in_to_serialize, out_stream);
	}

	template <typename T_key, typename T_value>
	void serialize(const std::unordered_map<T_key, T_value>& in_to_serialize, Serialize_stream& out_stream)
	{
		serialize_map(in_to_serialize, out_stream);
	}

	template <typename T_key, typename T_value, typename T_map>
	void deserialize_map(Deserialize_stream& in_stream, T_map& out_deserialized)
	{
		size_t len = 0;
		deserialize(in_stream, len);

		for (size_t i = 0; i < len; i++)
		{
			std::pair<T_key, T_value> pair;

			deserialize(in_stream, pair.first);
			deserialize(in_stream, pair.second);

			out_deserialized.insert(std::move(pair));
		}
	}

	template <typename T_key, typename T_value>
	void deserialize(Deserialize_stream& in_stream, std::map<T_key, T_value>& out_deserialized)
	{
		deserialize_map<T_key, T_value>(in_stream, out_deserialized);
	}

	template <typename T_key, typename T_value>
	void deserialize(Deserialize_stream& in_stream, std::unordered_map<T_key, T_value>& out_deserialized)
	{
		deserialize_map<T_key, T_value>(in_stream, out_deserialized);
	}

	template <>
	inline void serialize(const std::string& in_to_serialize, Serialize_stream& out_stream)
	{
		const size_t len = in_to_serialize.size();
		out_stream.write(len);

		const size_t start_buf = out_stream.m_bytes.size();

		out_stream.m_bytes.resize(start_buf + len);
		memcpy_s(&out_stream.m_bytes[start_buf], len, in_to_serialize.data(), len);
	}

	template <>
	inline void deserialize(Deserialize_stream& in_stream, std::string& out_deserialized)
	{
		size_t len = 0;
		in_stream.read(len);

		size_t old_len = out_deserialized.size();
		out_deserialized.resize(old_len + len);

		memcpy_s(&out_deserialized[old_len], len, &in_stream.m_bytes[in_stream.m_offset], len);

		in_stream.m_offset += static_cast<ui32>(len);
	}
}
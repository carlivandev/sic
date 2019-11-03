#pragma once
#include "defines.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <map>

namespace impuls
{
	struct serialize_stream
	{
		template <typename t_data_type>
		void write(const t_data_type& in)
		{
			serialize(in, *this);
		}

		std::string m_bytes;
	};

	struct deserialize_stream
	{
		deserialize_stream(std::string&& in_bytes) : m_bytes(in_bytes) {}

		template <typename t_data_type>
		void read(t_data_type& out)
		{
			deserialize(*this, out);
		}

		std::string m_bytes;
		ui32 m_offset = 0;
	};

	template <typename t_data_type>
	void serialize(const t_data_type& in_to_serialize, serialize_stream& out_stream)
	{
		const size_t old_size = out_stream.m_bytes.size();
		out_stream.m_bytes.resize(old_size + sizeof(t_data_type));
		memcpy_s(&out_stream.m_bytes[old_size], sizeof(t_data_type), &in_to_serialize, sizeof(t_data_type));
	}

	template <typename t_data_type>
	void deserialize(deserialize_stream& in_stream, t_data_type& out_deserialized)
	{
		memcpy_s(&out_deserialized, sizeof(t_data_type), &(in_stream.m_bytes[in_stream.m_offset]), sizeof(t_data_type));
		in_stream.m_offset += sizeof(t_data_type);
	}

	template <typename t_data_type>
	void serialize(const std::vector<t_data_type>& in_to_serialize, serialize_stream& out_stream)
	{
		serialize(in_to_serialize.size(), out_stream);

		for (const t_data_type& element : in_to_serialize)
			serialize(element, out_stream);
	}

	template <typename t_data_type>
	void deserialize(deserialize_stream& in_stream, std::vector<t_data_type>& out_deserialized)
	{
		const size_t old_len = out_deserialized.size();

		size_t len = 0;
		deserialize(in_stream, len);
		
		out_deserialized.resize(old_len + len);

		for (size_t i = old_len; i < old_len + len; i++)
			deserialize(in_stream, out_deserialized[i]);
	}

	template <typename t_map>
	void serialize_map(const t_map& in_to_serialize, serialize_stream& out_stream)
	{
		serialize(in_to_serialize.size(), out_stream);

		for (auto element : in_to_serialize)
		{
			serialize(element.first, out_stream);
			serialize(element.second, out_stream);
		}
	}

	template <typename t_key, typename t_value>
	void serialize(const std::map<t_key, t_value>& in_to_serialize, serialize_stream& out_stream)
	{
		serialize_map(in_to_serialize, out_stream);
	}

	template <typename t_key, typename t_value>
	void serialize(const std::unordered_map<t_key, t_value>& in_to_serialize, serialize_stream& out_stream)
	{
		serialize_map(in_to_serialize, out_stream);
	}

	template <typename t_key, typename t_value, typename t_map>
	void deserialize_map(deserialize_stream& in_stream, t_map& out_deserialized)
	{
		size_t len = 0;
		deserialize(in_stream, len);

		for (size_t i = 0; i < len; i++)
		{
			std::pair<t_key, t_value> pair;

			deserialize(in_stream, pair.first);
			deserialize(in_stream, pair.second);

			out_deserialized.insert(std::move(pair));
		}
	}

	template <typename t_key, typename t_value>
	void deserialize(deserialize_stream& in_stream, std::map<t_key, t_value>& out_deserialized)
	{
		deserialize_map<t_key, t_value>(in_stream, out_deserialized);
	}

	template <typename t_key, typename t_value>
	void deserialize(deserialize_stream& in_stream, std::unordered_map<t_key, t_value>& out_deserialized)
	{
		deserialize_map<t_key, t_value>(in_stream, out_deserialized);
	}

	template <>
	inline void serialize(const std::string& in_to_serialize, serialize_stream& out_stream)
	{
		const size_t len = in_to_serialize.size();
		out_stream.write(len);

		const size_t start_buf = out_stream.m_bytes.size();

		out_stream.m_bytes.resize(start_buf + len);
		memcpy_s(&out_stream.m_bytes[start_buf], len, in_to_serialize.data(), len);
	}

	template <>
	inline void deserialize(deserialize_stream& in_stream, std::string& out_deserialized)
	{
		size_t len = 0;
		in_stream.read(len);

		size_t old_len = out_deserialized.size();
		out_deserialized.resize(old_len + len);

		memcpy_s(&out_deserialized[old_len], len, &in_stream.m_bytes[in_stream.m_offset], len);

		in_stream.m_offset += static_cast<ui32>(len);
	}
}
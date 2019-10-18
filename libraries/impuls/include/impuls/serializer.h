#pragma once
#include "defines.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <map>

namespace impuls
{
	template <typename t_data_type>
	void serialize(const t_data_type& in_to_serialize, std::vector<impuls::byte>& out_serialized_data)
	{
		const size_t old_size = out_serialized_data.size();
		out_serialized_data.resize(old_size + sizeof(t_data_type));
		memcpy_s(&out_serialized_data[old_size], sizeof(t_data_type), &in_to_serialize, sizeof(t_data_type));
	}

	template <typename t_data_type>
	ui32 deserialize(const impuls::byte& in_serialized_data_begin, t_data_type& out_deserialized)
	{
		memcpy_s(&out_deserialized, sizeof(t_data_type), &in_serialized_data_begin, sizeof(t_data_type));
		return sizeof(t_data_type);
	}

	template <typename t_data_type>
	void serialize(const std::vector<t_data_type>& in_to_serialize, std::vector<impuls::byte>& out_serialized_data)
	{
		serialize(static_cast<ui32>(in_to_serialize.size()), out_serialized_data);

		for (const t_data_type& element : in_to_serialize)
			serialize(element, out_serialized_data);
	}

	template <typename t_data_type>
	ui32 deserialize(const impuls::byte& in_serialized_data_begin, std::vector<t_data_type>& out_deserialized)
	{
		const ui32& len = *reinterpret_cast<const ui32*>(&in_serialized_data_begin);
		out_deserialized.resize(len);

		ui32 current_byte = sizeof(ui32);

		for (ui32 i = 0; i < len; i++)
			current_byte += deserialize((&in_serialized_data_begin)[current_byte], out_deserialized[i]);

		return current_byte;
	}

	template <typename t_map>
	void serialize_map(const t_map& in_to_serialize, std::vector<impuls::byte>& out_serialized_data)
	{
		serialize(static_cast<ui32>(in_to_serialize.size()), out_serialized_data);

		for (auto element : in_to_serialize)
		{
			serialize(element.first, out_serialized_data);
			serialize(element.second, out_serialized_data);
		}
	}

	template <typename t_key, typename t_value>
	void serialize(const std::map<t_key, t_value>& in_to_serialize, std::vector<impuls::byte>& out_serialized_data)
	{
		serialize_map(in_to_serialize, out_serialized_data);
	}

	template <typename t_key, typename t_value>
	void serialize(const std::unordered_map<t_key, t_value>& in_to_serialize, std::vector<impuls::byte>& out_serialized_data)
	{
		serialize_map(in_to_serialize, out_serialized_data);
	}

	template <typename t_key, typename t_value, typename t_map>
	ui32 deserialize_map(const impuls::byte& in_serialized_data_begin, t_map& out_deserialized)
	{
		const ui32& len = *reinterpret_cast<const ui32*>(&in_serialized_data_begin);
		out_deserialized.clear();

		ui32 current_byte = sizeof(ui32);

		for (ui32 i = 0; i < len; i++)
		{
			std::pair<t_key, t_value> pair;

			current_byte += deserialize((&in_serialized_data_begin)[current_byte], pair.first);
			current_byte += deserialize((&in_serialized_data_begin)[current_byte], pair.second);

			out_deserialized.insert(std::move(pair));
		}

		return current_byte;
	}

	template <typename t_key, typename t_value>
	ui32 deserialize(const impuls::byte& in_serialized_data_begin, std::map<t_key, t_value>& out_deserialized)
	{
		return deserialize_map<t_key, t_value>(in_serialized_data_begin, out_deserialized);
	}

	template <typename t_key, typename t_value>
	ui32 deserialize(const impuls::byte& in_serialized_data_begin, std::unordered_map<t_key, t_value>& out_deserialized)
	{
		return deserialize_map<t_key, t_value>(in_serialized_data_begin, out_deserialized);
	}

	template <>
	inline void serialize(const std::string& in_to_serialize, std::vector<impuls::byte>& out_serialized_data)
	{
		const ui32 len = static_cast<ui32>(in_to_serialize.size());

		const size_t old_size = out_serialized_data.size();
		out_serialized_data.resize(old_size + len + sizeof(ui32));

		memcpy_s(&out_serialized_data[old_size], sizeof(ui32), &len, sizeof(ui32));
		memcpy_s(&out_serialized_data[old_size + sizeof(ui32)], len, in_to_serialize.data(), len);
	}

	template <>
	inline ui32 deserialize(const impuls::byte& in_serialized_data_begin, std::string& out_deserialized)
	{
		const ui32& len = *reinterpret_cast<const ui32*>(&in_serialized_data_begin);
		out_deserialized.resize(len);

		memcpy_s(&out_deserialized[0], len, &len + 1, len);
		return len + sizeof(ui32);
	}
}
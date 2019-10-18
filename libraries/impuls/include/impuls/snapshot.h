#pragma once
#include <unordered_map>
#include "defines.h"
#include "serializer.h"

namespace impuls
{
	struct snapshot_data
	{
		struct type_data
		{
			std::string m_key;
			std::vector<impuls::byte> m_data;
		};

		std::unordered_map<std::string, ui32> m_object_unique_key_to_type_index;
		std::unordered_map<std::string, ui32> m_component_unique_key_to_type_index;
		
		std::vector<type_data> m_object_type_datas;
		std::vector<type_data> m_component_type_datas;

		ui32 m_current_object_type_index = 0;
		ui32 m_current_component_type_index = 0;
	};

	template <>
	inline void serialize(const snapshot_data::type_data& in_to_serialize, std::vector<impuls::byte>& out_serialized_data)
	{
		serialize(in_to_serialize.m_key, out_serialized_data);
		serialize(in_to_serialize.m_data, out_serialized_data);
	}

	template <>
	inline ui32 deserialize(const impuls::byte& in_serialized_data_begin, snapshot_data::type_data& out_deserialized)
	{
		 const ui32 key_size = deserialize(in_serialized_data_begin, out_deserialized.m_key);
		 return key_size + deserialize((&in_serialized_data_begin)[key_size], out_deserialized.m_data);
	}
}
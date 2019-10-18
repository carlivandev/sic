#pragma once
#include "defines.h"
#include "type_index.h"
#include "serializer.h"
#include <functional>
#include <sstream>

namespace impuls
{
	struct world_context;

	struct property
	{
		template <typename t_data_type, typename t_owner>
		void set(t_owner& in_owner, const t_data_type& in_to_set)
		{
			assert(m_owner_typeindex == type_index<>::get<t_owner>() && "type mismatch, owner type incorrect");
			assert(m_data_typeindex == type_index<>::get<t_data_type>() && "type mismatch, data type incorrect");

			m_setter(*reinterpret_cast<impuls::byte*>(&in_owner), *reinterpret_cast<const impuls::byte*>(&in_to_set));
		}

		void set(byte& in_owner, const byte& in_to_set)
		{
			m_setter(in_owner, in_to_set);
		}

		template <typename t_data_type, typename t_owner>
		t_data_type& get(t_owner& in_owner)
		{
			assert(m_owner_typeindex == type_index<>::get<t_owner>() && "type mismatch, owner type incorrect");
			assert(m_data_typeindex == type_index<>::get<t_data_type>() && "type mismatch, data type incorrect");

			return *reinterpret_cast<t_data_type*>(&m_getter(*reinterpret_cast<impuls::byte*>(&in_owner)));
		}

		byte& get(byte& in_owner)
		{
			return m_getter(in_owner);
		}

		std::function<void(byte& in_to_set_on, const byte& in_data_to_set)> m_setter;
		std::function<byte&(byte& in_to_get_from)> m_getter;
		std::function<void(byte& in_to_serialize_from, std::vector<byte>& out_serialized_data)> m_serializer;
		std::function<ui32(const impuls::byte& in_serialized_data_begin, byte& out_deserialized)> m_deserializer;
		i32 m_owner_typeindex = -1;
		i32 m_data_typeindex = -1;
	};

	struct typeinfo
	{
		template <typename t_owner>
		std::vector<byte> serialize_instance(t_owner& in_owner)
		{
			std::vector<byte> serialized_data;

			for (auto prop : m_properties)
			{
				serialize(prop.first, serialized_data);
				prop.second.m_serializer(*reinterpret_cast<impuls::byte*>(&in_owner), serialized_data);

				serialized_data.push_back('\n');
			}

			return std::move(serialized_data);
		}

		template <typename t_owner>
		void deserialize_instance(const std::vector<byte>& in_serialized_data, t_owner& out_owner)
		{
// 			std::stringstream serialized_stream;
// 			serialized_stream.write(in_serialized_data.data(), in_serialized_data.size());
// 
// 			std::string line;
// 			while (std::getline(serialized_stream, line))
// 			{
// 				std::string prop_key;
// 				const ui32 prop_byte_begin = deserialize(line.data()[0], prop_key);
// 
// 				auto prop_it = m_properties.find(prop_key);
// 
// 				if (prop_it != m_properties.end())
// 					prop_it->second.m_deserializer(line.data()[prop_byte_begin], *reinterpret_cast<impuls::byte*>(&out_owner));
// 			}

			ui32 cur_idx = 0;
			std::string prop_key;

			while (cur_idx < in_serialized_data.size())
			{
				const ui32 prop_byte_begin = cur_idx + deserialize(in_serialized_data.data()[cur_idx], prop_key);

				auto prop_it = m_properties.find(prop_key);

				if (prop_it != m_properties.end())
				{
					const ui32 end_idx = prop_it->second.m_deserializer(in_serialized_data.data()[prop_byte_begin], *reinterpret_cast<impuls::byte*>(&out_owner));
					cur_idx = prop_byte_begin + end_idx + 1;
				}
			}
		}

		property* get(const char* in_key)
		{
			auto it = m_properties.find(in_key);

			if (it != m_properties.end())
				return &it->second;

			return nullptr;
		}

		std::string m_name;
		std::string m_unique_key;
		std::unordered_map<std::string, property> m_properties;
	};

	struct type_reg
	{
		type_reg(typeinfo* in_to_register) : m_to_register(in_to_register) {}

		template <typename t_owner, typename t_data_type>
		type_reg& property(const char* in_name, t_data_type t_owner::*in_ptr)
		{
			impuls::property& new_prop = m_to_register->m_properties[in_name];

			new_prop.m_setter = [in_ptr](byte& in_to_set_on, const byte& in_data_to_set)
			{
				t_owner& as_t = *reinterpret_cast<t_owner*>(&in_to_set_on);
				as_t.*in_ptr = *reinterpret_cast<const t_data_type*>(&in_data_to_set);
			};

			new_prop.m_getter = [in_ptr](byte& in_to_get_from) -> byte&
			{
				t_owner& as_t = *reinterpret_cast<t_owner*>(&in_to_get_from);
				return *reinterpret_cast<byte*>(&(as_t.*in_ptr));
			};

			new_prop.m_serializer = [in_ptr](byte& in_to_serialize_from, std::vector<byte>& out_serialized_data)
			{
				t_owner& as_t = *reinterpret_cast<t_owner*>(&in_to_serialize_from);

				serialize(as_t.*in_ptr, out_serialized_data);
			};

			new_prop.m_deserializer = [in_ptr](const impuls::byte& in_serialized_data_begin, byte& out_deserialized) -> ui32
			{
				t_owner& as_t = *reinterpret_cast<t_owner*>(&out_deserialized);

				return deserialize(in_serialized_data_begin, as_t.*in_ptr);
			};

			new_prop.m_owner_typeindex = type_index<>::get<t_owner>();
			new_prop.m_data_typeindex = type_index<>::get<t_data_type>();

			return *this;
		}

		typeinfo* m_to_register = nullptr;
	};
}
#pragma once
#include "defines.h"
#include "type_index.h"
#include "serializer.h"
#include <functional>
#include <sstream>

namespace sic
{
	struct Property
	{
		template <typename T_data_type, typename T_owner>
		void set(T_owner& in_owner, const T_data_type& in_to_set) const
		{
			assert(m_owner_typeindex == Type_index<>::get<T_owner>() && "type mismatch, owner type incorrect");
			assert(m_data_typeindex == Type_index<>::get<T_data_type>() && "type mismatch, data type incorrect");

			m_setter(*reinterpret_cast<sic::byte*>(&in_owner), *reinterpret_cast<const sic::byte*>(&in_to_set));
		}

		void set(byte& in_owner, const byte& in_to_set) const
		{
			m_setter(in_owner, in_to_set);
		}

		template <typename T_data_type, typename T_owner>
		T_data_type& get(T_owner& in_owner) const
		{
			assert(m_owner_typeindex == Type_index<>::get<T_owner>() && "type mismatch, owner type incorrect");
			assert(m_data_typeindex == Type_index<>::get<T_data_type>() && "type mismatch, data type incorrect");

			return *reinterpret_cast<T_data_type*>(&m_getter(*reinterpret_cast<sic::byte*>(&in_owner)));
		}

		byte& get(byte& in_owner) const
		{
			return m_getter(in_owner);
		}

		std::function<void(byte& in_to_set_on, const byte& in_data_to_set)> m_setter;
		std::function<byte&(byte& in_to_get_from)> m_getter;
		std::function<void(byte& in_to_serialize_from, std::vector<byte>& out_serialized_data)> m_serializer;
		std::function<ui32(const sic::byte& in_serialized_data_begin, byte& out_deserialized)> m_deserializer;
		i32 m_owner_typeindex = -1;
		i32 m_data_typeindex = -1;
	};

	struct Typeinfo
	{
		template <typename T_owner>
		std::vector<byte> serialize_instance(T_owner& in_owner) const
		{
			std::vector<byte> serialized_data;

// 			for (auto prop : m_properties)
// 			{
// 				serialize(prop.first, serialized_data);
// 				prop.second.m_serializer(*reinterpret_cast<sic::byte*>(&in_owner), serialized_data);
// 
// 				serialized_data.push_back('\n');
// 			}

			return std::move(serialized_data);
		}

		template <typename T_owner>
		void deserialize_instance(const std::vector<byte>& in_serialized_data, T_owner& out_owner) const
		{
// 			std::stringstream serialized_stream;
// 			serialized_stream.write(in_serialized_data.data(), in_serialized_data.get_max_elements());
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
// 					prop_it->second.m_deserializer(line.data()[prop_byte_begin], *reinterpret_cast<sic::byte*>(&out_owner));
// 			}

// 			ui32 cur_idx = 0;
// 			std::string prop_key;
// 
// 			while (cur_idx < in_serialized_data.size())
// 			{
// 				const ui32 prop_byte_begin = cur_idx + deserialize(in_serialized_data.data()[cur_idx], prop_key);
// 
// 				auto prop_it = m_properties.find(prop_key);
// 
// 				if (prop_it != m_properties.end())
// 				{
// 					const ui32 end_idx = prop_it->second.m_deserializer(in_serialized_data.data()[prop_byte_begin], *reinterpret_cast<sic::byte*>(&out_owner));
// 					cur_idx = prop_byte_begin + end_idx + 1;
// 				}
// 			}
		}

		const Property* get(const char* in_key) const
		{
			auto it = m_properties.find(in_key);

			if (it != m_properties.end())
				return &it->second;

			return nullptr;
		}

		std::string m_name;
		std::string m_unique_key;
		std::unordered_map<std::string, Property> m_properties;
	};

	struct Type_reg
	{
		Type_reg(Typeinfo* in_to_register) : m_to_register(in_to_register) {}

		template <typename T_owner, typename T_data_type>
		Type_reg& Property(const char* in_name, T_data_type T_owner::*in_ptr)
		{
			sic::Property& new_prop = m_to_register->m_properties[in_name];

			new_prop.m_setter = [in_ptr](byte& in_to_set_on, const byte& in_data_to_set)
			{
				T_owner& as_t = *reinterpret_cast<T_owner*>(&in_to_set_on);
				as_t.*in_ptr = *reinterpret_cast<const T_data_type*>(&in_data_to_set);
			};

			new_prop.m_getter = [in_ptr](byte& in_to_get_from) -> byte&
			{
				T_owner& as_t = *reinterpret_cast<T_owner*>(&in_to_get_from);
				return *reinterpret_cast<byte*>(&(as_t.*in_ptr));
			};

// 			new_prop.m_serializer = [in_ptr](byte& in_to_serialize_from, std::vector<byte>& out_serialized_data)
// 			{
// 				T_owner& as_t = *reinterpret_cast<T_owner*>(&in_to_serialize_from);
// 
// 				serialize(as_t.*in_ptr, out_serialized_data);
// 			};
// 
// 			new_prop.m_deserializer = [in_ptr](const sic::byte& in_serialized_data_begin, byte& out_deserialized) -> ui32
// 			{
// 				T_owner& as_t = *reinterpret_cast<T_owner*>(&out_deserialized);
// 
// 				return deserialize(in_serialized_data_begin, as_t.*in_ptr);
// 			};

			new_prop.m_owner_typeindex = Type_index<>::get<T_owner>();
			new_prop.m_data_typeindex = Type_index<>::get<T_data_type>();

			return *this;
		}

		Typeinfo* m_to_register = nullptr;
	};
}
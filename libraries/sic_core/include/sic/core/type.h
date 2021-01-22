#pragma once
#include "sic/core/defines.h"
#include "sic/core/serializer.h"

#include <functional>
#include <any>

namespace sic
{
	namespace rtti
	{
		struct Field
		{
			Field(const std::type_info& in_owner_type, const std::type_info& in_data_type) : m_owner_typeindex(in_owner_type), m_data_typeindex(in_data_type) {}

			template <typename T_data_type, typename T_owner>
			__forceinline void set(T_owner& in_owner, const T_data_type& in_to_set) const
			{
				assert(m_owner_typeindex == typeid(T_owner) && "type mismatch, owner type incorrect");
				assert(m_data_typeindex == typeid(T_data_type) && "type mismatch, data type incorrect");

				m_setter(*reinterpret_cast<sic::byte*>(&in_owner), *reinterpret_cast<const sic::byte*>(&in_to_set));
			}

			__forceinline void set(byte& in_owner, const byte& in_to_set) const
			{
				m_setter(in_owner, in_to_set);
			}

			template <typename T_owner>
			__forceinline std::any get(T_owner& in_owner) const
			{
				assert(m_owner_typeindex == typeid(T_owner) && "type mismatch, owner type incorrect");

				std::any ret_val;
				m_getter(*reinterpret_cast<sic::byte*>(&in_owner), ret_val);
				return ret_val;
			}

			__forceinline std::any get(byte& in_owner) const
			{
				std::any ret_val;
				m_getter(in_owner, ret_val);
				return ret_val;
			}

			std::function<void(byte& in_to_set_on, const byte& in_data_to_set)> m_setter;
			std::function<void(byte& in_to_get_from, std::any& out_any)> m_getter;
			std::function<void(const byte& in_to_serialize_from, Serialize_stream& out_stream)> m_serializer;
			std::function<void(Deserialize_stream& inout_stream, byte& out_deserialized)> m_deserializer;
			const std::type_info& m_owner_typeindex;
			const std::type_info& m_data_typeindex;
		};

		struct Typeinfo
		{
			template <typename T_owner>
			void serialize_instance(const T_owner& in_owner, Serialize_stream& inout_stream) const
			{				
				serialize(m_fields.size(), inout_stream);

				for (auto&& field : m_fields)
				{
					serialize(field.first, inout_stream);
					field.second.m_serializer(*reinterpret_cast<const sic::byte*>(&in_owner), inout_stream);
				}
			}

			template <typename T_owner>
			void deserialize_instance(Deserialize_stream& inout_stream, T_owner& out_owner) const
			{
				size_t num_fields;
				deserialize(inout_stream, num_fields);

				for (size_t i = 0; i < num_fields; ++i)
				{
					std::string key;
					deserialize(inout_stream, key);
					if (const Field* field = get_field(key.c_str()))
						field->m_deserializer(inout_stream, *reinterpret_cast<sic::byte*>(&out_owner));
				}
			}

			const Field* get_field(const char* in_key) const
			{
				auto it = m_fields.find(in_key);

				if (it != m_fields.end())
					return &it->second;

				return nullptr;
			}

			std::string m_name;
			std::string m_unique_key;
			std::unordered_map<std::string, Field> m_fields;
		};

		struct Type_reg
		{
			Type_reg(Typeinfo* in_to_register) : m_to_register(in_to_register) {}

			template <typename T_owner, typename T_data_type>
			Type_reg& field(const char* in_name, T_data_type T_owner::* in_ptr)
			{
				auto new_field = m_to_register->m_fields.emplace(in_name, Field(typeid(T_owner), typeid(T_data_type)));
				
				new_field.first->second.m_setter = [in_ptr](byte& in_to_set_on, const byte& in_data_to_set)
				{
					T_owner& as_t = *reinterpret_cast<T_owner*>(&in_to_set_on);
					as_t.*in_ptr = *reinterpret_cast<const T_data_type*>(&in_data_to_set);
				};

				new_field.first->second.m_getter = [in_ptr](byte& in_to_get_from, std::any& out_any)
				{
					T_owner& as_t = *reinterpret_cast<T_owner*>(&in_to_get_from);
					out_any = (as_t.*in_ptr);
				};

				new_field.first->second.m_serializer = [in_ptr](const byte& in_to_serialize_from, Serialize_stream& out_stream)
				{
					const T_owner& as_t = *reinterpret_cast<const T_owner*>(&in_to_serialize_from);
				
					serialize(as_t.*in_ptr, out_stream);
				};
				
				new_field.first->second.m_deserializer = [in_ptr](Deserialize_stream& inout_stream, byte& out_deserialized)
				{
					T_owner& as_t = *reinterpret_cast<T_owner*>(&out_deserialized);
					deserialize(inout_stream, as_t.*in_ptr);
				};

				return *this;
			}

			Typeinfo* m_to_register = nullptr;
		};

		template <typename T>
		constexpr void register_type(Type_reg&& in_registrator)
		{
			in_registrator;
		}
	}
}
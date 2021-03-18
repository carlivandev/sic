#pragma once
#include "sic/core/defines.h"
#include "sic/core/serializer.h"

#include <functional>
#include <any>

namespace sic
{
	namespace rtti
	{
		struct Typeinfo;

		struct Rtti
		{
			template <typename T_type_to_register>
			__forceinline constexpr void register_typeinfo(const char* in_unique_key);

			template <typename T_type>
			__forceinline constexpr const rtti::Typeinfo* find_typeinfo() const;

		private:
			std::vector<rtti::Typeinfo*> m_typeinfos;
			std::vector<rtti::Typeinfo*> m_component_typeinfos;
			std::vector<rtti::Typeinfo*> m_object_typeinfos;
			std::vector<rtti::Typeinfo*> m_state_typeinfos;
			std::unordered_map<std::string, std::unique_ptr<rtti::Typeinfo>> m_typename_to_typeinfo_lut;
		};

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
			std::function<void(const Rtti& in_rtti, const byte& in_to_serialize_from, Serialize_stream& out_stream)> m_serializer;
			std::function<void(const Rtti& in_rtti, Deserialize_stream& inout_stream, byte& out_deserialized)> m_deserializer;
			const std::type_info& m_owner_typeindex;
			const std::type_info& m_data_typeindex;
		};

		struct Typeinfo
		{
			template <typename T_owner>
			void serialize_instance(const Rtti& in_rtti, const T_owner& in_owner, Serialize_stream& inout_stream) const
			{				
				serialize(m_fields.size(), inout_stream);

				for (auto&& field : m_fields)
				{
					serialize(field.first, inout_stream);
					field.second.m_serializer(in_rtti, *reinterpret_cast<const sic::byte*>(&in_owner), inout_stream);
				}
			}

			template <typename T_owner>
			void deserialize_instance(const Rtti& in_rtti, Deserialize_stream& inout_stream, T_owner& out_owner) const
			{
				size_t num_fields;
				deserialize(inout_stream, num_fields);

				for (size_t i = 0; i < num_fields; ++i)
				{
					std::string key;
					deserialize(inout_stream, key);
					if (const Field* field = get_field(key.c_str()))
						field->m_deserializer(in_rtti, inout_stream, *reinterpret_cast<sic::byte*>(&out_owner));
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

		template <typename T_data_type>
		void serialize_rtti(const Rtti& in_rtti, const T_data_type& in_to_serialize_from, Serialize_stream& out_stream)
		{
			if (const Typeinfo* other_typeinfo = in_rtti.find_typeinfo<T_data_type>())
				other_typeinfo->serialize_instance(in_rtti, in_to_serialize_from, out_stream);
			else
				serialize(in_to_serialize_from, out_stream);
		}

		template <typename T_data_type>
		void serialize_rtti(const Rtti& in_rtti, const std::vector<T_data_type>& in_to_serialize_from, Serialize_stream& out_stream)
		{
			serialize(in_to_serialize_from.size(), out_stream);

			for (auto&& to_serialize : in_to_serialize_from)
				serialize_rtti(in_rtti, to_serialize, out_stream);
		}

		template <typename T_data_type>
		void deserialize_rtti(const Rtti& in_rtti, Deserialize_stream& inout_stream, T_data_type& out_deserialized)
		{
			if (const Typeinfo* other_typeinfo = in_rtti.find_typeinfo<T_data_type>())
				other_typeinfo->deserialize_instance(in_rtti, inout_stream, out_deserialized);
			else
				deserialize(inout_stream, out_deserialized);
		}

		template <typename T_data_type>
		void deserialize_rtti(const Rtti& in_rtti, Deserialize_stream& inout_stream, std::vector<T_data_type>& out_deserialized)
		{
			size_t size = 0;
			deserialize(inout_stream, size);
			out_deserialized.reserve(size);

			for (size_t i = 0; i < size; i++)
			{
				T_data_type new_item;
				deserialize_rtti(in_rtti, inout_stream, new_item);
				out_deserialized.emplace_back(new_item);
			}
		}

		template <typename T_map>
		void serialize_map_rtti(const Rtti& in_rtti, const T_map& in_to_serialize, Serialize_stream& out_stream)
		{
			serialize(in_to_serialize.size(), out_stream);

			for (auto element : in_to_serialize)
			{
				serialize_rtti(in_rtti, element.first, out_stream);
				serialize_rtti(in_rtti, element.second, out_stream);
			}
		}

		template <typename T_key, typename T_value>
		void serialize_rtti(const Rtti& in_rtti, const std::map<T_key, T_value>& in_to_serialize, Serialize_stream& out_stream)
		{
			serialize_map_rtti(in_rtti, in_to_serialize, out_stream);
		}

		template <typename T_key, typename T_value>
		void serialize_rtti(const Rtti& in_rtti, const std::unordered_map<T_key, T_value>& in_to_serialize, Serialize_stream& out_stream)
		{
			serialize_map_rtti(in_rtti, in_to_serialize, out_stream);
		}

		template <typename T_key, typename T_value, typename T_map>
		void deserialize_map_rtti(const Rtti& in_rtti, Deserialize_stream& in_stream, T_map& out_deserialized)
		{
			size_t len = 0;
			deserialize(in_stream, len);

			for (size_t i = 0; i < len; i++)
			{
				std::pair<T_key, T_value> pair;

				deserialize_rtti(in_rtti, in_stream, pair.first);
				deserialize_rtti(in_rtti, in_stream, pair.second);

				out_deserialized.insert(std::move(pair));
			}
		}

		template <typename T_key, typename T_value>
		void deserialize_rtti(const Rtti& in_rtti, Deserialize_stream& in_stream, std::map<T_key, T_value>& out_deserialized)
		{
			deserialize_map_rtti<T_key, T_value>(in_rtti, in_stream, out_deserialized);
		}

		template <typename T_key, typename T_value>
		void deserialize_rtti(const Rtti& in_rtti, Deserialize_stream& in_stream, std::unordered_map<T_key, T_value>& out_deserialized)
		{
			deserialize_map_rtti<T_key, T_value>(in_rtti, in_stream, out_deserialized);
		}

		template <typename T_value_0, typename T_value_1>
		void serialize_rtti(const Rtti& in_rtti, const std::pair<T_value_0, T_value_1>& in_to_serialize, Serialize_stream& out_stream)
		{
			serialize_rtti(in_rtti, in_to_serialize.first, out_stream);
			serialize_rtti(in_rtti, in_to_serialize.second, out_stream);
		}

		template <typename T_value_0, typename T_value_1>
		void deserialize_rtti(const Rtti& in_rtti, Deserialize_stream& in_stream, std::pair<T_value_0, T_value_1>& out_deserialized)
		{
			deserialize_rtti(in_rtti, in_stream, out_deserialized.first);
			deserialize_rtti(in_rtti, in_stream, out_deserialized.second);
		}

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

				new_field.first->second.m_serializer = [in_ptr](const Rtti& in_rtti, const byte& in_to_serialize_from, Serialize_stream& out_stream)
				{
					const T_owner& as_t = *reinterpret_cast<const T_owner*>(&in_to_serialize_from);
					serialize_rtti(in_rtti, as_t.*in_ptr, out_stream);
				};
				
				new_field.first->second.m_deserializer = [in_ptr](const Rtti& in_rtti, Deserialize_stream& inout_stream, byte& out_deserialized)
				{
					T_owner& as_t = *reinterpret_cast<T_owner*>(&out_deserialized);
					deserialize_rtti(in_rtti, inout_stream, as_t.*in_ptr);
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

		template<typename T_type_to_register>
		__forceinline constexpr void Rtti::register_typeinfo(const char* in_unique_key)
		{
			const char* type_name = typeid(T_type_to_register).name();

			assert(m_typename_to_typeinfo_lut.find(type_name) == m_typename_to_typeinfo_lut.end() && "typeinfo already registered!");

			auto& new_typeinfo = m_typename_to_typeinfo_lut[type_name] = std::make_unique<rtti::Typeinfo>();
			new_typeinfo->m_name = type_name;
			new_typeinfo->m_unique_key = in_unique_key;

			constexpr bool is_component = std::is_base_of<Component_base, T_type_to_register>::value;
			constexpr bool is_object = std::is_base_of<Object_base, T_type_to_register>::value;
			constexpr bool is_state = std::is_base_of<State, T_type_to_register>::value;

			if constexpr (is_component)
			{
				const ui32 type_idx = Type_index<Component_base>::get<T_type_to_register>();

				while (type_idx >= m_component_typeinfos.size())
					m_component_typeinfos.push_back(nullptr);

				m_component_typeinfos[type_idx] = new_typeinfo.get();
			}
			else if constexpr (is_object)
			{
				const ui32 type_idx = Type_index<Object_base>::get<T_type_to_register>();

				while (type_idx >= m_object_typeinfos.size())
					m_object_typeinfos.push_back(nullptr);

				m_object_typeinfos[type_idx] = new_typeinfo.get();
			}
			else if constexpr (is_state)
			{
				const ui32 type_idx = Type_index<State>::get<T_type_to_register>();

				while (type_idx >= m_state_typeinfos.size())
					m_state_typeinfos.push_back(nullptr);

				m_state_typeinfos[type_idx] = new_typeinfo.get();
			}
			else
			{
				const i32 type_idx = Type_index<rtti::Typeinfo>::get<T_type_to_register>();

				while (type_idx >= m_typeinfos.size())
					m_typeinfos.push_back(nullptr);

				m_typeinfos[type_idx] = new_typeinfo.get();
			}

			rtti::register_type<T_type_to_register>(std::move(rtti::Type_reg(new_typeinfo.get())));
		}

		template<typename T_type>
		__forceinline constexpr const rtti::Typeinfo* Rtti::find_typeinfo() const
		{
			constexpr bool is_component = std::is_base_of<Component_base, T_type>::value;
			constexpr bool is_object = std::is_base_of<Object_base, T_type>::value;
			constexpr bool is_state = std::is_base_of<State, T_type>::value;

			if constexpr (is_component)
			{
				const ui32 type_idx = Type_index<Component_base>::get<T_type>();
				if (type_idx < m_component_typeinfos.size() && m_component_typeinfos[type_idx] != nullptr)
					return m_component_typeinfos[type_idx];

				return nullptr;
			}
			else if constexpr (is_object)
			{
				const ui32 type_idx = Type_index<Object_base>::get<T_type>();
				if (type_idx < m_object_typeinfos.size() && m_object_typeinfos[type_idx] != nullptr)
					return m_object_typeinfos[type_idx];

				return nullptr;
			}
			else if constexpr (is_state)
			{
				const ui32 type_idx = Type_index<State>::get<T_type>();
				if (type_idx < m_states.size() && m_states[type_idx] != nullptr)
					return m_states[type_idx];

				return nullptr;
			}
			else
			{
				const ui32 type_idx = Type_index<rtti::Typeinfo>::get<T_type>();
				if (type_idx < m_typeinfos.size() && m_typeinfos[type_idx] != nullptr)
					return m_typeinfos[type_idx];

				return nullptr;
			}
		}
}
}
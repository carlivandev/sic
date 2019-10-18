#pragma once
#include "bucket_allocator_view.h"
#include "world.h"
#include "object.h"
#include "type.h"
#include "snapshot.h"

namespace impuls
{
	struct i_object_base;
	
	struct world_context
	{
		world_context(world& in_world) : m_world(&in_world) {}

		template <typename t_component_type>
		__forceinline constexpr t_component_type& create_component(i_object_base& in_object_to_attach_to)
		{
			constexpr bool is_valid_type = std::is_base_of<i_component_base, t_component_type>::value;

			static_assert(is_valid_type, "can only create types that derive i_component_base");

			return m_world->create_component<t_component_type>(in_object_to_attach_to);
		}

		template <typename t_component_type>
		__forceinline constexpr void destroy_component(t_component_type& in_component_to_destroy)
		{
			constexpr bool is_valid_type = std::is_base_of<i_component_base, t_component_type>::value;

			static_assert(is_valid_type, "can only destroy types that derive i_component_base");

			return m_world->destroy_component<t_component_type>(in_component_to_destroy);
		}

		template <typename t_component>
		__forceinline constexpr type_reg register_component_type(const char* in_unique_key, ui32 in_initial_capacity = 128, ui32 in_bucket_capacity = 64)
		{
			m_world->register_component_type<t_component>(in_initial_capacity, in_bucket_capacity);

			return std::move(register_typeinfo<t_component>(in_unique_key));
		}

		template <typename t_object>
		__forceinline constexpr type_reg register_object(const char* in_unique_key, ui32 in_initial_capacity = 128, ui32 in_bucket_capacity = 64)
		{
			static_assert(std::is_base_of<i_object_base, t_object>::value, "object must derive from struct i_object<>");

			const ui32 type_idx = type_index<i_object_base>::get<t_object>();

			auto& new_object_storage = m_world->get_object_storage_at_index(type_idx);

			assert(new_object_storage.get() == nullptr && "object is already registered");

			new_object_storage = std::make_unique<object_storage>();
			reinterpret_cast<object_storage*>(new_object_storage.get())->initialize_with_typesize(in_initial_capacity, in_bucket_capacity, sizeof(t_object));

			return std::move(register_typeinfo<t_object>(in_unique_key));
		}

		template <typename t_object>
		__forceinline constexpr t_object& create_object_instance()
		{
			static_assert(std::is_base_of<i_object_base, t_object>::value, "object must derive from struct i_object<>");

			const ui32 type_idx = type_index<i_object_base>::get<t_object>();

			assert((type_idx < m_world->m_objects.size() || m_world->m_objects[type_idx].get() != nullptr) && "type not registered");

			auto& arch_to_create_from = m_world->m_objects[type_idx];

			t_object& new_instance = reinterpret_cast<object_storage*>(arch_to_create_from.get())->make_instance<t_object>(*m_world);

			
			m_world->invoke<event_created<t_object>>(new_instance);

			return new_instance;
		}

		template <typename t_object>
		__forceinline constexpr void destroy_object_instance(t_object& in_object_instance_to_destroy)
		{
			static_assert(std::is_base_of<i_object_base, t_object>::value, "object must derive from struct i_object<>");

			const ui32 type_idx = type_index<i_object_base>::get<t_object>();

			assert((type_idx < m_world->m_objects.size() || m_world->m_objects[type_idx].get() != nullptr) && "type not registered");

			auto* arch_to_destroy_from = reinterpret_cast<object_storage*>(m_world->m_objects[type_idx].get());

			if (in_object_instance_to_destroy.m_parent)
			{
				for (i32 i = 0; i < in_object_instance_to_destroy.m_parent->m_children.size(); i++)
				{
					if (in_object_instance_to_destroy.m_parent->m_children[i] == &in_object_instance_to_destroy)
					{
						in_object_instance_to_destroy.m_parent->m_children[i] = in_object_instance_to_destroy.m_parent->m_children.back();
						in_object_instance_to_destroy.m_parent->m_children.pop_back();
						break;
					}
				}
			}

			for (i32 i = 0; i < in_object_instance_to_destroy.m_children.size(); i++)
			{
				i_object_base* child = in_object_instance_to_destroy.m_children[i];

				reinterpret_cast<object_storage*>(m_world->m_objects[child->m_type_index].get())->destroy_instance(*m_world, *child);
			}

			m_world->invoke<event_destroyed<t_object>>(in_object_instance_to_destroy);
			arch_to_destroy_from->destroy_instance(*m_world, in_object_instance_to_destroy);
		}

		void add_child(i_object_base& in_parent, i_object_base& in_child);
		void unchild(i_object_base& in_child);

		template <typename t_state>
		__forceinline constexpr type_reg register_state(const char* in_unique_key)
		{
			const ui32 type_idx = type_index<i_state>::get<t_state>();

			auto& state_to_register = m_world->get_state_at_index(type_idx);

			assert(state_to_register == nullptr && "state already registered!");

			state_to_register = std::make_unique<t_state>();

			return std::move(register_typeinfo<t_state>(in_unique_key));
		}

		template <typename t_type_to_register>
		__forceinline constexpr type_reg register_typeinfo(const char* in_unique_key)
		{
			const char* type_name = typeid(t_type_to_register).name();

			assert(m_world->m_typename_to_typeinfo_lut.find(type_name) == m_world->m_typename_to_typeinfo_lut.end() && "typeinfo already registered!");

			auto& new_typeinfo = m_world->m_typename_to_typeinfo_lut[type_name] = std::make_unique<typeinfo>();
			new_typeinfo->m_name = type_name;
			new_typeinfo->m_unique_key = in_unique_key;

			constexpr bool is_component = std::is_base_of<i_component_base, t_type_to_register>::value;
			constexpr bool is_object = std::is_base_of<i_object_base, t_type_to_register>::value;
			constexpr bool is_state = std::is_base_of<i_state, t_type_to_register>::value;

			if constexpr (is_component)
			{
				const ui32 type_idx = type_index<i_component_base>::get<t_type_to_register>();
				
				while (type_idx >= m_world->m_component_typeinfos.size())
					m_world->m_component_typeinfos.push_back(nullptr);

				m_world->m_component_typeinfos[type_idx] = new_typeinfo.get();
			}
			else if constexpr (is_object)
			{
				const ui32 type_idx = type_index<i_object_base>::get<t_type_to_register>();
				
				while (type_idx >= m_world->m_object_typeinfos.size())
					m_world->m_object_typeinfos.push_back(nullptr);

				m_world->m_object_typeinfos[type_idx] = new_typeinfo.get();
			}
			else if constexpr (is_state)
			{
				const ui32 type_idx = type_index<i_state>::get<t_type_to_register>();
				
				while (type_idx >= m_world->m_state_typeinfos.size())
					m_world->m_state_typeinfos.push_back(nullptr);

				m_world->m_state_typeinfos[type_idx] = new_typeinfo.get();
			}
			else
			{
				const i32 type_idx = type_index<typeinfo>::get<t_type_to_register>();

				while (type_idx >= m_world->m_typeinfos.size())
					m_world->m_typeinfos.push_back(nullptr);

				m_world->m_typeinfos[type_idx] = new_typeinfo.get();
			}

			return std::move(type_reg(new_typeinfo.get()));
		}

		template <typename t_type>
		__forceinline constexpr typeinfo* get_typeinfo()
		{
			constexpr bool is_component = std::is_base_of<i_component_base, t_type>::value;
			constexpr bool is_object = std::is_base_of<i_object_base, t_type>::value;
			constexpr bool is_state = std::is_base_of<i_state, t_type>::value;

			if constexpr (is_component)
			{
				const ui32 type_idx = type_index<i_component_base>::get<t_type>();
				assert(type_idx < m_world->m_component_typeinfos.size() && m_world->m_component_typeinfos[type_idx] != nullptr && "typeinfo not registered!");

				return m_world->m_component_typeinfos[type_idx];
			}
			else if constexpr (is_object)
			{
				const ui32 type_idx = type_index<i_object_base>::get<t_type>();
				assert(type_idx < m_world->m_object_typeinfos.size() && m_world->m_object_typeinfos[type_idx] != nullptr && "typeinfo not registered!");

				return m_world->m_object_typeinfos[type_idx];
			}
			else if constexpr (is_state)
			{
				const ui32 type_idx = type_index<i_state>::get<t_type>();
				assert(type_idx < m_world->m_states.size() && m_world->m_states[type_idx] != nullptr && "typeinfo not registered!");

				return m_world->m_states[type_idx];
			}
			else
			{
				const ui32 type_idx = type_index<typeinfo>::get<t_type>();
				assert(type_idx < m_world->m_typeinfos.size() && m_world->m_typeinfos[type_idx] != nullptr && "typeinfo not registered!");

				return m_world->m_typeinfos[type_idx];
			}
		}

		template <typename t_base_type>
		__forceinline constexpr typeinfo* get_typeinfo(const t_base_type& in_base_type)
		{
			constexpr bool is_component = std::is_base_of<i_component_base, t_base_type>::value;
			constexpr bool is_object = std::is_base_of<i_object_base, t_base_type>::value;

			const ui32 type_idx = in_base_type.m_type_index;

			if constexpr (is_component)
			{
				assert(type_idx < m_world->m_component_typeinfos.size() && m_world->m_component_typeinfos[type_idx] != nullptr && "typeinfo not registered!");

				return m_world->m_component_typeinfos[type_idx];
			}
			else if constexpr (is_object)
			{
				assert(type_idx < m_world->m_object_typeinfos.size() && m_world->m_object_typeinfos[type_idx] != nullptr && "typeinfo not registered!");

				return m_world->m_object_typeinfos[type_idx];
			}
			else
			{
				static_assert(false, "cant get typeinfo of that base type");
			}
		}

		template <typename t_type>
		__forceinline bucket_allocator_view<t_type> each()
		{
			constexpr bool is_component = std::is_base_of<i_component_base, t_type>::value;
			constexpr bool is_object = std::is_base_of<i_object_base, t_type>::value;

			if constexpr (is_component)
			{
				const ui32 type_idx = type_index<i_component_base>::get<t_type>();
				return std::move(bucket_allocator_view<t_type>(&m_world->m_component_storages[type_idx]->m_components));
			}
			else if constexpr (is_object)
			{
				const ui32 type_idx = type_index<i_object_base>::get<t_type>();
				return std::move(bucket_allocator_view<t_type>(&m_world->m_objects[type_idx]->m_instances.m_byte_allocator));
			}
			else
			{
				static_assert(is_component || is_object, "can not get each of type");
			}
		}

		template <typename t_type>
		__forceinline bucket_allocator_view<const t_type> each() const
		{
			constexpr bool is_component = std::is_base_of<i_component_base, t_type>::value;
			constexpr bool is_object = std::is_base_of<i_object_base, t_type>::value;

			if constexpr (is_component)
			{
				const ui32 type_idx = type_index<i_component_base>::get<t_type>();
				return std::move(bucket_allocator_view<const t_type>(&m_world->m_component_storages[type_idx]->m_components));
			}
			else if constexpr (is_object)
			{
				const ui32 type_idx = type_index<i_object_base>::get<t_type>();
				return std::move(bucket_allocator_view<const t_type>(&m_world->m_objects[type_idx]->m_instances.m_byte_allocator));
			}
			else
			{
				static_assert(is_component || is_object, "can not get each of type");
			}
		}

		template <typename t_state>
		__forceinline t_state* get_state()
		{
			constexpr bool is_valid_type = std::is_base_of<i_state, t_state>::value;

			static_assert(is_valid_type, "can only get types that derive i_state");

			const i32 type_idx = type_index<i_state>::get<t_state>();

			assert((type_idx < m_world->m_states.size() && m_world->m_states[type_idx].get() != nullptr) && "state not registered");

			return reinterpret_cast<t_state*>(m_world->m_states[type_idx].get());
		}

		template <typename t_event_type, typename t_functor>
		void listen(t_functor in_func)
		{
			m_world->listen<t_event_type, t_functor>(std::move(in_func));
		}

		template <typename t_event_type, typename event_data>
		void invoke(event_data& event_data_to_send)
		{
			m_world->invoke<t_event_type, event_data>(event_data_to_send);
		}

		void snapshot(std::vector<byte>& out_snapshot)
		{
			snapshot_data data;
			
			for (ui32 obj_storage_idx = 0; obj_storage_idx < m_world->m_objects.size(); obj_storage_idx++)
			{
				std::unique_ptr<i_object_storage_base>& storage = m_world->m_objects[obj_storage_idx];

				bucket_allocator& allocator = reinterpret_cast<object_storage*>(storage.get())->m_instances;

				for (ui32 i = 0; i < allocator.m_byte_allocator.size(); i += allocator.m_byte_allocator.m_typesize)
				{
					i_object_base* as_obj = reinterpret_cast<i_object_base*>(&allocator.m_byte_allocator[i]);
					//as_obj->snapshot(out_snapshot, obj_storage_idx);
					as_obj;
				}
			}

			/*

			1. for each component type, create its own vector with data

			2. for each object type create its own vector with data

			3. serialized object is just its parent index and children index

			4. serialized component is object type index, object instance index, and lastly its actual component data

			ex.
			shape
			-1 empty_vec (first shape)
			-1 empty_vec (second shape)

			shape_data
			0 0 <instance0> (owner type idx, owner instance idx, component data)
			0 1 <instance1> (owner type idx, owner instance idx, component data)

			*/

			serialize(data.m_object_type_datas, out_snapshot);
			serialize(data.m_component_type_datas, out_snapshot);
		}

		void load_snapshot(const std::vector<byte>& in_snapshot)
		{
			//first make sure world has no objects
			for (std::unique_ptr<i_object_storage_base>& storage : m_world->m_objects)
			{
				bucket_allocator& allocator = reinterpret_cast<object_storage*>(storage.get())->m_instances;

				for (ui32 i = 0; i < allocator.m_byte_allocator.size(); i += allocator.m_byte_allocator.m_typesize)
				{
					i_object_base* as_obj = reinterpret_cast<i_object_base*>(&allocator.m_byte_allocator[i]);
					as_obj->destroy_instance(*m_world);
				}
			}

			//then, load
			snapshot_data data;
			const ui32 end_idx = deserialize(in_snapshot[0], data.m_object_type_datas);
			deserialize(in_snapshot[end_idx], data.m_component_type_datas);
		}

		world* m_world = nullptr;
	};
}


#pragma once
#include "bucket_allocator_view.h"
#include "world.h"
#include "object.h"
#include "type.h"

namespace impuls
{
	struct i_object_base;
	
	struct world_context
	{
		world_context(world& in_world) : m_world(&in_world) {}
		world_context(world& in_world, i_system& in_system) : m_world(&in_world), m_current_system(&in_system) {}

		template<typename t_system_type>
		__forceinline t_system_type& create_subsystem()
		{
			auto& new_system = m_world->create_system<t_system_type>();
			m_current_system->m_subsystems.push_back(&new_system);

			return new_system;
		}

		template <typename t_component>
		__forceinline constexpr type_reg register_component_type(const char* in_unique_key, ui32 in_initial_capacity = 128)
		{
			m_world->register_component_type<t_component>(in_initial_capacity);

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
		__forceinline constexpr t_object& create_object()
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
		__forceinline constexpr void destroy_object(t_object& in_object_to_destroy)
		{
			static_assert(std::is_base_of<i_object_base, t_object>::value, "object must derive from struct i_object<>");

			const ui32 type_idx = in_object_to_destroy.m_type_index;

			assert((type_idx < m_world->m_objects.size() || m_world->m_objects[type_idx].get() != nullptr) && "type not registered");

			auto* arch_to_destroy_from = reinterpret_cast<object_storage*>(m_world->m_objects[type_idx].get());

			if (in_object_to_destroy.m_parent)
			{
				for (i32 i = 0; i < in_object_to_destroy.m_parent->m_children.size(); i++)
				{
					if (in_object_to_destroy.m_parent->m_children[i] == &in_object_to_destroy)
					{
						in_object_to_destroy.m_parent->m_children[i] = in_object_to_destroy.m_parent->m_children.back();
						in_object_to_destroy.m_parent->m_children.pop_back();
						break;
					}
				}
			}

			for (i32 i = 0; i < in_object_to_destroy.m_children.size(); i++)
			{
				i_object_base* child = in_object_to_destroy.m_children[i];

				reinterpret_cast<object_storage*>(m_world->m_objects[child->m_type_index].get())->destroy_instance(*m_world, *child);
			}

			arch_to_destroy_from->destroy_instance(*m_world, in_object_to_destroy);
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
		__forceinline plf::colony<t_type>& components()
		{
			const ui32 type_idx = type_index<i_component_base>::get<t_type>();
			component_storage<t_type>* storage = reinterpret_cast<component_storage<t_type>*>(m_world->m_component_storages[type_idx].get());

			return storage->m_components;
		}

		template <typename t_type>
		__forceinline const plf::colony<t_type>& components() const
		{
			const ui32 type_idx = type_index<i_component_base>::get<t_type>();
			const component_storage<t_type>* storage = reinterpret_cast<const component_storage<t_type>*>(m_world->m_component_storages[type_idx].get());

			return storage->m_components;
		}

		template <typename t_type>
		__forceinline bucket_allocator_view<t_type> objects()
		{
			const ui32 type_idx = type_index<i_object_base>::get<t_type>();
			return std::move(bucket_allocator_view<t_type>(&m_world->m_objects[type_idx]->m_instances.m_byte_allocator));
		}

		template <typename t_type>
		__forceinline bucket_allocator_view<const t_type> objects() const
		{
			const ui32 type_idx = type_index<i_object_base>::get<t_type>();
			return std::move(bucket_allocator_view<const t_type>(&m_world->m_objects[type_idx]->m_instances.m_byte_allocator));
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

		world* m_world = nullptr;
		i_system* m_current_system = nullptr;
	};
}


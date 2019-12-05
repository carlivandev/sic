#pragma once
#include "impuls/level.h"
#include "impuls/bucket_allocator_view.h"

namespace impuls
{
	struct i_object_base;
	
	struct level_context
	{
		level_context(engine& inout_engine, level& inout_level) : m_engine(inout_engine), m_level(inout_level){}

		template <typename t_object>
		__forceinline constexpr t_object& create_object()
		{
			static_assert(std::is_base_of<i_object_base, t_object>::value, "object must derive from struct i_object<>");

			const ui32 type_idx = type_index<i_object_base>::get<t_object>();

			assert((type_idx < m_level.m_objects.size() || m_level.m_objects[type_idx].get() != nullptr) && "type not registered");

			auto& arch_to_create_from = m_level.m_objects[type_idx];

			t_object& new_instance = reinterpret_cast<object_storage*>(arch_to_create_from.get())->make_instance<t_object>(*this);

			
			m_engine.invoke<event_created<t_object>>(new_instance);

			return new_instance;
		}

		template <typename t_object>
		__forceinline constexpr void destroy_object(t_object& in_object_to_destroy)
		{
			static_assert(std::is_base_of<i_object_base, t_object>::value, "object must derive from struct i_object<>");

			const ui32 type_idx = in_object_to_destroy.m_type_index;

			assert((type_idx < m_level.m_objects.size() || m_level.m_objects[type_idx].get() != nullptr) && "type not registered");

			auto* arch_to_destroy_from = reinterpret_cast<object_storage*>(m_level.m_objects[type_idx].get());

			//TODO: this should be recursive

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

				reinterpret_cast<object_storage*>(m_level.m_objects[child->m_type_index].get())->destroy_instance(*this, *child);
			}

			arch_to_destroy_from->destroy_instance(*this, in_object_to_destroy);
		}

		void add_child(i_object_base& in_parent, i_object_base& in_child);
		void unchild(i_object_base& in_child);

		template<typename t_type>
		__forceinline void for_each(std::function<void(t_type&)> in_func)
		{
			m_level.for_each<t_type>(in_func);
		}

		template<typename t_type>
		__forceinline void for_each(std::function<void(const t_type&)> in_func) const
		{
			m_level.for_each<t_type>(in_func);
		}

		engine& m_engine;
		level& m_level;
	};
}


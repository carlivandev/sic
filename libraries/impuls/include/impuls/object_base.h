#pragma once
#include "impuls/type_restrictions.h"

#include "bucket_allocator.h"
#include "type_index.h"

namespace impuls
{
	struct Engine;
	struct Component_base;

	struct Object_base : Noncopyable
	{
		friend struct Engine_context;
		friend struct Level_context;
		friend struct Level;
		friend struct System;
		friend struct Object_storage;

		bool is_valid() const
		{
			return m_type_index != -1;
		}

		template <typename t_component>
		constexpr t_component* find()
		{
			const i32 type_idx = Type_index<Component_base>::get<t_component>();

			return reinterpret_cast<t_component*>(find_internal(type_idx));
		}

		template <typename t_component>
		constexpr const t_component* find() const
		{
			const i32 type_idx = Type_index<Component_base>::get<t_component>();

			return reinterpret_cast<const t_component*>(find_internal(type_idx));
		}

		void add_child(Object_base& inout_child)
		{
			for (Object_base* child : m_children)
			{
				if (child == &inout_child)
					return;
			}

			inout_child.unchild();

			m_children.push_back(&inout_child);

			inout_child.m_parent = this;
		}

		void unchild()
		{
			if (m_parent)
			{
				for (size_t i = 0; i < m_parent->m_children.size(); i++)
				{
					if (m_parent->m_children[i] == this)
					{
						if (i + 1 < m_parent->m_children.size())
							m_parent->m_children[i] = m_parent->m_children.back();

						m_parent->m_children.pop_back();
						break;
					}
				}

				m_parent = nullptr;
			}
		}

	protected:
		virtual byte* find_internal(i32 in_type_idx) = 0;
		virtual const byte* find_internal(i32 in_type_idx) const = 0;
		virtual void destroy_instance(Level_context& inout_level) = 0;

		std::vector<Object_base*> m_children;
		Object_base* m_parent = nullptr;
		i32 m_type_index = -1;
	};

	struct Object_storage_base
	{
		bucket_allocator m_instances;
	};
}

#pragma once
#include "sic/core/type_restrictions.h"

#include "bucket_allocator.h"
#include "type_index.h"

namespace sic
{
	struct Engine;
	struct Component_base;

	struct Object_id
	{
		Object_id() = default;
		Object_id(i32 in_object_id, i32 in_scene_id) : m_id(in_object_id), m_scene_id(in_scene_id) {}

		i32 get_id() const { return m_id; }
		i32 get_scene_id() const { return m_scene_id; }

		bool get_is_set() const { return m_id != -1 && m_scene_id != -1; }

	private:
		i32 m_id = -1;
		i32 m_scene_id = -1;
	};

	struct Object_base : Noncopyable
	{
		friend struct Engine_context;
		friend struct Scene_context;
		friend struct Scene;
		friend struct System;
		friend struct Object_storage;

		bool is_valid() const
		{
			return m_type_index != -1 && !m_pending_destroy;
		}

		constexpr const i32 get_scene_id() const
		{
			return m_scene_id;
		}
		
		constexpr const i32 get_outermost_scene_id() const
		{
			return m_outermost_scene_id;
		}

		Object_id get_id() const
		{
			return Object_id(m_id, m_scene_id);
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
		template <typename T_component>
		constexpr T_component* find()
		{
			const i32 type_idx = Type_index<Component_base>::get<T_component>();

			return reinterpret_cast<T_component*>(find_internal(type_idx));
		}

		template <typename T_component>
		constexpr const T_component* find() const
		{
			const i32 type_idx = Type_index<Component_base>::get<T_component>();

			return reinterpret_cast<const T_component*>(find_internal(type_idx));
		}

		virtual byte* find_internal(i32 in_type_idx) = 0;
		virtual const byte* find_internal(i32 in_type_idx) const = 0;
		virtual void destroy_instance(Scene_context& inout_scene) = 0;

		std::vector<Object_base*> m_children;
		Object_base* m_parent = nullptr;
		i32 m_type_index = -1;
		i32 m_id = -1;
		i32 m_scene_id = -1;
		i32 m_outermost_scene_id = -1;
		bool m_pending_destroy = false;
	};

	struct Object_storage_base
	{
		bucket_allocator m_instances;
	};
}

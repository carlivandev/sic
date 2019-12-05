#pragma once
#include "impuls/type_restrictions.h"

#include "bucket_allocator.h"
#include "type_index.h"

namespace impuls
{
	struct engine;
	struct i_component_base;

	struct i_object_base : i_noncopyable
	{
		friend struct engine_context;
		friend struct level_context;
		friend struct i_system;
		friend struct object_storage;

		bool is_valid() const
		{
			return m_type_index != -1;
		}

		template <typename t_component>
		constexpr t_component* find()
		{
			const i32 type_idx = type_index<i_component_base>::get<t_component>();

			return reinterpret_cast<t_component*>(find_internal(type_idx));
		}

		template <typename t_component>
		constexpr const t_component* find() const
		{
			const i32 type_idx = type_index<i_component_base>::get<t_component>();

			return reinterpret_cast<const t_component*>(find_internal(type_idx));
		}

		virtual byte* find_internal(i32 in_type_idx) = 0;
		virtual const byte* find_internal(i32 in_type_idx) const = 0;

		protected:
			virtual void destroy_instance(level_context& inout_level) = 0;

			std::vector<i_object_base*> m_children;
			i_object_base* m_parent = nullptr;
			i32 m_type_index = -1;
	};

	struct i_object_storage_base
	{
		bucket_allocator m_instances;
	};
}

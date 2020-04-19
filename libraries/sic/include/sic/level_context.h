#pragma once
#include "sic/level.h"
#include "sic/bucket_allocator_view.h"
#include "sic/object.h"
#include "sic/engine_context.h"

namespace sic
{
	struct Object_base;
	
	struct Level_context
	{
		template <typename t_subtype, typename ...t_component>
		friend struct Object;

		Level_context(Engine& inout_engine, Level& inout_level) : m_engine(inout_engine), m_level(inout_level){}

		template <typename t_object>
		__forceinline constexpr t_object& create_object()
		{
			return m_level.create_object<t_object>();
		}

		void destroy_object(Object_base& in_object_to_destroy)
		{
			m_level.destroy_object(in_object_to_destroy);
		}

		template<typename T_type>
		__forceinline void for_each(std::function<void(T_type&)> in_func)
		{
			m_level.for_each<T_type>(in_func);
		}

		template<typename T_type>
		__forceinline void for_each(std::function<void(const T_type&)> in_func) const
		{
			m_level.for_each<T_type>(in_func);
		}

		Engine_context get_engine_context() { return m_engine; }
		const Engine_context get_engine_context() const { return m_engine; }
		
		i32 get_outermost_level_id() const { return m_level.m_outermost_level != nullptr ? m_level.m_outermost_level->m_level_id : get_level_id(); }
		i32 get_level_id() const { return m_level.m_level_id; }

	private:
		Engine& m_engine;
		Level& m_level;
	};
}


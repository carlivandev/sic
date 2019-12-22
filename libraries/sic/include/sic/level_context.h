#pragma once
#include "sic/level.h"
#include "sic/bucket_allocator_view.h"
#include "sic/object.h"

namespace sic
{
	struct Object_base;
	
	struct Level_context
	{
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

		Engine& m_engine;
		Level& m_level;
	};
}


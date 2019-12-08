#pragma once
#include "impuls/level.h"
#include "impuls/bucket_allocator_view.h"
#include "impuls/object.h"

namespace impuls
{
	struct i_object_base;
	
	struct level_context
	{
		level_context(engine& inout_engine, level& inout_level) : m_engine(inout_engine), m_level(inout_level){}

		template <typename t_object>
		__forceinline constexpr t_object& create_object()
		{
			return m_level.create_object<t_object>();
		}

		void destroy_object(i_object_base& in_object_to_destroy)
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

		engine& m_engine;
		level& m_level;
	};
}


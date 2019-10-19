#pragma once
#include "defines.h"
#include <type_traits>

namespace impuls
{
	template <typename t_base = void>
	struct type_index
	{
		template <typename t_derived>
		__forceinline static i32 get()
		{
			static i32 id = m_id_ticker++;

			return id;
		}

		static i32 m_id_ticker;
	};

	template <typename t_base>
	i32 type_index<t_base>::m_id_ticker = 0;
}
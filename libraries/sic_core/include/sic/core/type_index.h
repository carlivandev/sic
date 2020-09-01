#pragma once
#include "defines.h"
#include <type_traits>

namespace sic
{
	template <typename T_base = void>
	struct Type_index
	{
		template <typename T_derived>
		__forceinline static i32 get()
		{
			static i32 id = m_id_ticker++;

			return id;
		}

		static i32 m_id_ticker;
	};

	template <typename T_base>
	i32 Type_index<T_base>::m_id_ticker = 0;
}
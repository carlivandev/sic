#pragma once

#include <cstdint>
#include <limits>

namespace sic
{
	using byte = char;
	
	using i8 =		int8_t;
	using i16 =		int16_t;
	using i32 =		int32_t;
	using i64 =		int64_t;

	using ui8 =		uint8_t;
	using ui16 =	uint16_t;
	using ui32 =	uint32_t;
	using ui64 =	uint64_t;

	namespace constants
	{
		constexpr float pi = 3.14159265f;
	}
}
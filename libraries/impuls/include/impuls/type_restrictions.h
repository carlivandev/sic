#pragma once

namespace impuls
{
	struct i_noncopyable
	{
		i_noncopyable() = default;
		~i_noncopyable() = default;

		i_noncopyable(const i_noncopyable&) = delete;
		i_noncopyable& operator=(const i_noncopyable&) = delete;

		i_noncopyable(i_noncopyable&&) = default;
		i_noncopyable& operator=(i_noncopyable&&) = default;
	};
}
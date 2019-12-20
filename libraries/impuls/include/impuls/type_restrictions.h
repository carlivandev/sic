#pragma once

namespace sic
{
	struct Noncopyable
	{
		Noncopyable() = default;
		~Noncopyable() = default;

		Noncopyable(const Noncopyable&) = delete;
		Noncopyable& operator=(const Noncopyable&) = delete;

		Noncopyable(Noncopyable&&) = default;
		Noncopyable& operator=(Noncopyable&&) = default;
	};
}
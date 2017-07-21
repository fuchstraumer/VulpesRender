#pragma once
#ifndef VULPES_VK_NON_COPYABLE_H
#define VULPES_VK_NON_COPYABLE_H

namespace vulpes {

	class NonCopyable {

		constexpr NonCopyable(const NonCopyable& other) = delete;
		constexpr NonCopyable& operator=(const NonCopyable& other) = delete;

	protected:
		
		constexpr NonCopyable() noexcept = default;
		constexpr NonCopyable(NonCopyable&&) noexcept = default;
		constexpr NonCopyable& operator=(NonCopyable&& other) noexcept = default;

	};


	class NonMovable {
		constexpr NonMovable(const NonMovable&) = delete;
		constexpr NonMovable& operator =(const NonMovable&) = delete;
		constexpr NonMovable(NonMovable&&) = delete;
		constexpr NonMovable& operator=(NonMovable&&) = delete;

	protected:
		constexpr NonMovable() noexcept = default;
	};
}

#endif // !VULPES_VK_NON_COPYABLE_H

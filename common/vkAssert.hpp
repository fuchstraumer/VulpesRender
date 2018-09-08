#pragma once
#ifndef VK_ASSERT_H
#define VK_ASSERT_H
#include <iostream>
#include <cassert>
#include "vulkan/vulkan.h"
#if !defined NDEBUG || defined VK_FORCE_ASSERT

#define VkAssert(expression) { vkErrCheck((expression), __FILE__, __LINE__); }

inline void vkErrCheck(VkResult res, const char* file, unsigned line, bool abort = true) {
	if (res != VK_SUCCESS) {
		std::cerr << "VkAssert: error " << res << " at " << file << " line " << line << "\n";
		if (abort) {
			throw std::runtime_error("VkAssert failure!");
		}
	}
}

#else 

#define VkAssert(expression) ((void)(0))

#endif // ndef NDEBUG || defined VK_FORCE_ASSERT

#endif // !VK_ASSERT_H

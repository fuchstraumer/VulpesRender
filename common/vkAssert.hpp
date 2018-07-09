#pragma once
#ifndef VK_ASSERT_H
#define VK_ASSERT_H
#include <iostream>
#include <string>
#include "vulkan/vulkan.h"
#if !defined NDEBUG || defined VK_FORCE_ASSERT

#define VkAssert(expression) { vkErrCheck((expression), __FILE__, __LINE__); }

inline void vkErrCheck(VkResult res, const char* file, unsigned line, bool abort = true) {
	if (res != VK_SUCCESS) {
		std::cerr << "VkAssert: error " << std::to_string(res) << " at " << file << " line " << std::to_string(line) << "\n";
		if (abort) {
			throw std::runtime_error(std::to_string(res));
		}
	}
}

#else 

#define VkAssert(expression) ((void)(0))

#endif // ndef NDEBUG || defined VK_FORCE_ASSERT

#endif // !VK_ASSERT_H

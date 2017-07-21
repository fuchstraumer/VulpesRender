#pragma once
#ifndef VK_ASSERT_H
#define VK_ASSERT_H
#include <iostream>
#include "vulkan\vulkan.h"
#ifdef VK_FORCE_ASSERT
#define VkAssert(expression) { vkErrCheck((expression), __FILE__, __LINE__); }
inline void vkErrCheck(VkResult res, const char* file, unsigned line, bool abort = true) {
	if (res != VK_SUCCESS) {
		fprintf(stderr, "VkAssert: error %i at %s line %d \n", res, file, line);
		if (abort) {
			throw(res);
		}
	}
}
#else 
#ifdef NDEBUG 
#define VkAssert(expression) ((void)(0))
#else 
#define VkAssert(expression) { vkErrCheck((expression), __FILE__, __LINE__); }
inline void vkErrCheck(VkResult res, const char* file, unsigned line, bool abort = true) {
	if (res != VK_SUCCESS) {
		fprintf(stderr, "VkAssert: error %i at %s line %d \n", res, file, line);
		if (abort) {
			throw(res);
		}
	}
}
#endif 
#endif // VK_FORCE_ASSERT


#endif // !VK_ASSERT_H

#pragma once
#ifndef VULPES_VK_CONSTANTS_H
#define VULPES_VK_CONSTANTS_H

#include "vulkan/vulkan.h"
#include <iostream>

/*

	Debug callback stuff.

*/
namespace vpr {
	

	constexpr const char* standard_validation_layer = "VK_LAYER_LUNARG_standard_validation";

	constexpr std::array<const char*, 1> validation_layers = {
		"VK_LAYER_LUNARG_standard_validation"
	};


	constexpr const char* debug_callback_extension = "VK_EXT_debug_report";
	constexpr const char* khr_swapchain_extension = "VK_KHR_swapchain";

	constexpr std::array<const char*, 2> instance_extensions = {
		debug_callback_extension,
		khr_swapchain_extension,
	};

	constexpr std::array<const char*, 1> device_extensions = {
		"VK_KHR_swapchain",
		
	};

	constexpr std::array<const char*, 2> device_extensions_debug = {
		"VK_KHR_swapchain",
		"VK_EXT_debug_marker",
	};

	constexpr uint64_t vk_default_fence_timeout = std::numeric_limits<uint64_t>::max();

	enum class Platform {
		WINDOWS,
		LINUX,
		ANDROID,
	};

	enum class MovementDir {
		FORWARD,
		BACKWARD, 
		LEFT,
		RIGHT,
	};
}
#endif // !VULPES_VK_CONSTANTS_H

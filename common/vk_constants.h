#pragma once
#ifndef VULPES_VK_CONSTANTS_H
#define VULPES_VK_CONSTANTS_H

#include "vulkan\vulkan.h"
#include <iostream>

/*

	Debug callback stuff.

*/
namespace vulpes {
	

	constexpr char* standard_validation_layer = "VK_LAYER_LUNARG_standard_validation";

	constexpr std::array<const char*, 1> validation_layers = {
		standard_validation_layer,
	};


	constexpr char* debug_callback_extension = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
	constexpr char* khr_swapchain_extension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

	constexpr std::array<const char*, 2> instance_extensions = {
		debug_callback_extension,
		khr_swapchain_extension,
	};

	constexpr std::array<const char*, 1> device_extensions = {
		khr_swapchain_extension,

	};

	constexpr std::array<const char*, 2> device_extensions_debug = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_EXT_DEBUG_MARKER_EXTENSION_NAME,
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

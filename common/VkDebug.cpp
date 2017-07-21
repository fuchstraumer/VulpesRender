#include "stdafx.h"
#include "VkDebug.h"

void vulpes::DestroyDebugCallback(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks * allocation_callbacks) {
	auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	if (func != nullptr) {
		func(instance, callback, allocation_callbacks);
	}
}

void vulpes::CreateDebugCallback(const VkInstance & instance, const VkDebugReportFlagsEXT & flags, VkDebugReportCallbackEXT * callback, const VkAllocationCallbacks* allocators){

	auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
	if (func == nullptr) {
		std::cerr << "VKDEBUG::CREATE_DEBUG_CALLBACK: Debug layer and/or extension not enabled for VkInstance given in CreateDebugCallback." << std::endl;
		throw(std::runtime_error("Debug layer and/or extension not enabled for VkInstance given in CreateDebugCallback"));
	}

	VkDebugReportCallbackCreateInfoEXT create_info = vk_debug_callback_create_info_base;
	create_info.flags = flags;
	create_info.pfnCallback = VkBaseDebugCallbackPFN;

	VkResult err = func(instance, &create_info, allocators, callback);
	VkAssert(err);
}

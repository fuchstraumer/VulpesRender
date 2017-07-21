#pragma once
#ifndef VULPES_VK_DEBUG_H
#define VULPES_VK_DEBUG_H

#include "stdafx.h"

namespace vulpes {

	static VKAPI_ATTR VkBool32 VKAPI_CALL VkBaseDebugCallbackPFN(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData) {

		std::string prefix("");

		if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
			prefix += "ERROR:";
		}
		if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
			prefix += "WARNING:";
		}
		if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
			prefix += "INFORMATION:";
		}
		if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
			prefix += "PERFORMANCE_WARNING:";
		}
		// These come from the loader and validation layers: prefixed with VK_DEBUG to represent state as debug info from within vulkan itself
		if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
			prefix += "VK_DEBUG:";
		}
		
		// I apologize in advance
		std::string obj_type;
		switch (objType) {
		case VkDebugReportObjectTypeEXT::VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT:
			obj_type += "VK_INSTANCE";
			break;
		case VkDebugReportObjectTypeEXT::VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT:
			obj_type += "VK_LOGICAL_DEVICE";
			break;
		case VkDebugReportObjectTypeEXT::VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT:
			obj_type += "VK_PHYSICAL_DEVICE";
			break;
		case VkDebugReportObjectTypeEXT::VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT:
			obj_type += "VK_QUEUE";
			break;
		case VkDebugReportObjectTypeEXT::VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT:
			obj_type += "VK_SEMAPHORE";
			break;
		case VkDebugReportObjectTypeEXT::VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT:
			obj_type += "VK_COMMAND_BUFFER";
			break;
		case VkDebugReportObjectTypeEXT::VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT:
			obj_type += "VK_FENCE";
			break;
		case VkDebugReportObjectTypeEXT::VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT:
			obj_type += "VK_DEVICE_MEMORY";
			break;
		case VkDebugReportObjectTypeEXT::VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT:
			obj_type += "VK_BUFFER";
			break;
		case VkDebugReportObjectTypeEXT::VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT:
			obj_type += "VK_IMAGE";
			break;
		case VkDebugReportObjectTypeEXT::VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT:
			obj_type += "VK_EVENT";
			break;
		case VkDebugReportObjectTypeEXT::VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT:
			obj_type += "VK_QUERY_POOL";
			break;
		case VkDebugReportObjectTypeEXT::VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT:
			obj_type += "VK_BUFFER_VIEW";
			break;
		case VkDebugReportObjectTypeEXT::VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT:
			obj_type += "VK_SHADER_MODULE";
			break;
		case VkDebugReportObjectTypeEXT::VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT:
			obj_type += "VK_PIPELINE_CACHE";
			break;
		case VkDebugReportObjectTypeEXT::VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT:
			obj_type += "VK_PIPELINE_LAYOUT";
			break;
		case VkDebugReportObjectTypeEXT::VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT:
			obj_type += "VK_RENDER_PASS";
			break;
		case VkDebugReportObjectTypeEXT::VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT:
			obj_type += "VK_PIPELINE";
			break;
		case VkDebugReportObjectTypeEXT::VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT:
			obj_type += "VK_DESCRIPTOR_SET_LAYOUT";
			break;
		case VkDebugReportObjectTypeEXT::VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT:
			obj_type += "VK_SAMPLER";
			break;
		case VkDebugReportObjectTypeEXT::VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT:
			obj_type += "VK_DESCRIPTOR_POOL";
			break;
		case VkDebugReportObjectTypeEXT::VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT:
			obj_type += "VK_DESCRIPTOR_SET";
			break;
		case VkDebugReportObjectTypeEXT::VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT:
			obj_type += "VK_FRAMEBUFFER";
			break;
		case VkDebugReportObjectTypeEXT::VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT:
			obj_type += "VK_COMMAND_POOL";
			break;
		case VkDebugReportObjectTypeEXT::VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT:
			obj_type += "VK_SURFACE_KHR";
			break;
		case VkDebugReportObjectTypeEXT::VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT:
			obj_type += "VK_SWAPCHAIN_KHR";
			break;
		case VkDebugReportObjectTypeEXT::VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_EXT:
			obj_type += "VK_DEBUG_REPORT_EXT";
			break;
		default:
			obj_type += "OBJECT_TYPE_NOT_FOUND";
			break;
		}

		std::stringstream debug_msg;
		debug_msg << prefix << " [" << layerPrefix << "]" << " [" << obj_type << "]\n";
		debug_msg << "Obj. Handle: " << std::to_string(obj) << " Code: " << code << " at " << std::to_string(location) << " : " << msg << "\n";
		std::cout << debug_msg.str();
		return VK_FALSE;
	}


	void DestroyDebugCallback(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* allocation_callbacks = nullptr) {
		auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
		if (func != nullptr) {
			func(instance, callback, allocation_callbacks);
		}
	}

	void CreateDebugCallback(const VkInstance& instance, const VkDebugReportFlagsEXT& flags, VkDebugReportCallbackEXT* callback, const VkAllocationCallbacks* allocators = nullptr) {

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

}

#endif // !VULPES_VK_DEBUG_H
